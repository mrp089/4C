/*!----------------------------------------------------------------------
\file fluid3_surface_evaluate.cpp
\brief

Integrate a Surface Neumann boundary condition on a given boundary
element (tri or quad)

<pre>
Maintainer: Peter Gamnitzer
            gamnitzer@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15235
</pre>

*----------------------------------------------------------------------*/
#ifdef D_FLUID3
#ifdef CCADISCRET

#include "fluid3.H"
#include "../drt_lib/linalg_utils.H"
#include "../drt_lib/drt_utils.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_dserror.H"
#include "../drt_lib/drt_timecurve.H"

#include "../drt_mat/newtonianfluid.H"
#include "../drt_mat/carreauyasuda.H"
#include "../drt_mat/modpowerlaw.H"

using namespace DRT::UTILS;

/*----------------------------------------------------------------------*
 |  evaluate the element (public)                            g.bau 03/07|
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::Fluid3Surface::Evaluate(     ParameterList&            params,
                                                DRT::Discretization&      discretization,
                                                vector<int>&              lm,
                                                Epetra_SerialDenseMatrix& elemat1,
                                                Epetra_SerialDenseMatrix& elemat2,
                                                Epetra_SerialDenseVector& elevec1,
                                                Epetra_SerialDenseVector& elevec2,
                                                Epetra_SerialDenseVector& elevec3)
{
    DRT::ELEMENTS::Fluid3Surface::ActionType act = Fluid3Surface::none;
    string action = params.get<string>("action","none");
    if (action == "none") dserror("No action supplied");
    else if (action == "integrate_Shapefunction")
        act = Fluid3Surface::integrate_Shapefunction;
    else if (action == "flowrate calculation")
        act = Fluid3Surface::flowratecalc;
    else if (action == "Outlet impedance")
        act = Fluid3Surface::Outletimpedance;
    else if (action == "calc_node_normal")
        act = Fluid3Surface::calc_node_normal;
    else dserror("Unknown type of action for Fluid3_Surface");

    switch(act)
    {
    case integrate_Shapefunction:
    {
      RefCountPtr<const Epetra_Vector> dispnp;
      vector<double> mydispnp;

      if (parent_->IsAle())
      {
        dispnp = discretization.GetState("dispnp");
        if (dispnp!=null)
        {
          mydispnp.resize(lm.size());
          DRT::UTILS::ExtractMyValues(*dispnp,mydispnp,lm);
        }
      }

      IntegrateShapeFunction(params,discretization,lm,elevec1,mydispnp);
      break;
    }
    case flowratecalc:
    {
        FlowRateParameterCaculation(params,discretization,lm,elevec1);
        break;
    }
    case Outletimpedance:
    {
        ImpedanceIntegration(params,discretization,lm,elevec1);
        break;
    }
    case calc_node_normal:
    {
      RefCountPtr<const Epetra_Vector> dispnp;
      vector<double> mydispnp;

      if (parent_->IsAle())
      {
        dispnp = discretization.GetState("dispnp");
        if (dispnp!=null)
        {
          mydispnp.resize(lm.size());
          DRT::UTILS::ExtractMyValues(*dispnp,mydispnp,lm);
        }
      }
      ElementNodeNormal(params,discretization,lm,elevec1,mydispnp);
      break;
    }
    default:
        dserror("Unknown type of action for Fluid3_Surface");
    } // end of switch(act)

    return 0;
}

/*----------------------------------------------------------------------*
 |  Integrate a Surface Neumann boundary condition (public)  gammi 04/07|
 *----------------------------------------------------------------------*/
int DRT::ELEMENTS::Fluid3Surface::EvaluateNeumann(
                                           ParameterList& params,
                                           DRT::Discretization&      discretization,
                                           DRT::Condition&           condition,
                                           vector<int>&              lm,
                                           Epetra_SerialDenseVector& elevec1)
{
  // there are 3 velocities and 1 pressure
  const int numdf = 4;

  const double thsl = params.get("thsl",0.0);

  const DiscretizationType distype = this->Shape();

  double density; // density of my parent element

  // get material of volume element this surface belongs to
  RefCountPtr<MAT::Material> mat = parent_->Material();

  if( mat->MaterialType()    != m_carreauyasuda
      && mat->MaterialType() != m_modpowerlaw
      && mat->MaterialType() != m_fluid)
          dserror("Material law is not a fluid");

  MATERIAL* actmat = NULL;

  if(mat->MaterialType()== m_fluid)
  {
    actmat = static_cast<MAT::NewtonianFluid*>(mat.get())->MaterialData();
    density = actmat->m.fluid->density;
  }
  else if(mat->MaterialType()== m_carreauyasuda)
  {
    actmat = static_cast<MAT::CarreauYasuda*>(mat.get())->MaterialData();
    density = actmat->m.carreauyasuda->density;
  }
  else if(mat->MaterialType()== m_modpowerlaw)
  {
    actmat = static_cast<MAT::ModPowerLaw*>(mat.get())->MaterialData();
    density = actmat->m.modpowerlaw->density;
  }
  else
    dserror("fluid material expected but got type %d", mat->MaterialType());

  double invdensity = 1.0/density; // we shall need the inverse of rho

  // find out whether we will use a time curve
  bool usetime = true;
  const double time = params.get("total time",-1.0);
  if (time<0.0) usetime = false;

  // find out whether we will use a time curve and get the factor
  const vector<int>* curve  = condition.Get<vector<int> >("curve");
  int curvenum = -1;
  if (curve) curvenum = (*curve)[0];
  double curvefac = 1.0;
  if (curvenum>=0 && usetime)
    curvefac = DRT::UTILS::TimeCurveManager::Instance().Curve(curvenum).f(time);

  // get values and switches from the condition
  const vector<int>*    onoff = condition.Get<vector<int> >   ("onoff");
  const vector<double>* val   = condition.Get<vector<double> >("val"  );

  // set number of nodes
  const int iel   = this->NumNode();

  GaussRule2D  gaussrule = intrule2D_undefined;
  switch(distype)
  {
  case quad4:
      gaussrule = intrule_quad_4point;
      break;
  case quad8: case quad9:
      gaussrule = intrule_quad_9point;
      break;
  case tri3 :
      gaussrule = intrule_tri_3point;
      break;
  case tri6:
      gaussrule = intrule_tri_6point;
      break;
  default:
      dserror("shape type unknown!\n");
  }

  // allocate vector for shape functions and matrix for derivatives
  Epetra_SerialDenseVector  funct       (iel);
  Epetra_SerialDenseMatrix 	deriv       (2,iel);

  // node coordinates
  Epetra_SerialDenseMatrix      xyze        (3,iel);

  // the metric tensor and the area of an infintesimal surface element
  Epetra_SerialDenseMatrix 	metrictensor(2,2);
  double                        drs;

  // get node coordinates
  for(int i=0;i<iel;i++)
  {
    xyze(0,i)=this->Nodes()[i]->X()[0];
    xyze(1,i)=this->Nodes()[i]->X()[1];
    xyze(2,i)=this->Nodes()[i]->X()[2];
  }

  /*----------------------------------------------------------------------*
  |               start loop over integration points                     |
  *----------------------------------------------------------------------*/
  const IntegrationPoints2D  intpoints = getIntegrationPoints2D(gaussrule);
  for (int gpid=0; gpid<intpoints.nquad; gpid++)
  {
    const double e0 = intpoints.qxg[gpid][0];
    const double e1 = intpoints.qxg[gpid][1];

    // get shape functions and derivatives in the plane of the element
    shape_function_2D(funct, e0, e1, distype);
    shape_function_2D_deriv1(deriv, e0, e1, distype);

    // compute measure tensor for surface element and the infinitesimal
    // area element drs for the integration
    f3_metric_tensor_for_surface(xyze,deriv,metrictensor,&drs);

    // values are multiplied by the product from inf. area element,
    // the gauss weight, the timecurve factor and the constant
    // belonging to the time integration algorithm (theta*dt for
    // one step theta, 2/3 for bdf with dt const.)
    // further our equation is normalised by the density, hence we need to 
    // normalise also our rhs contribution
    const double fac = intpoints.qwgt[gpid] * drs * curvefac * thsl * invdensity;

    for (int node=0;node<iel;++node)
    {
      for(int dim=0;dim<3;dim++)
      {
        elevec1[node*numdf+dim]+=
          funct[node] * (*onoff)[dim] * (*val)[dim] * fac;
      }
    }

  } /* end of loop over integration points gpid */

  return 0;
}


/* compute kovariant metric tensor G for fluid element        gammi 04/07

                        +-       -+
                        | g11 g12 |
                    G = |         |
                        | g12 g22 |
                        +-       -+

 where (o denotes the inner product, xyz a vector)


                            dxyz   dxyz
                    g11 =   ---- o ----
                             dr     dr

                            dxyz   dxyz
                    g12 =   ---- o ----
                             dr     ds

                            dxyz   dxyz
                    g22 =   ---- o ----
                             ds     ds


 and the square root of the first fundamental form


                          +--------------+
                         /               |
           sqrtdetg =   /  g11*g22-g12^2
                      \/

 they are needed for the integration over the surface element

*/
void  DRT::ELEMENTS::Fluid3Surface::f3_metric_tensor_for_surface(
  const Epetra_SerialDenseMatrix  xyze,
  const Epetra_SerialDenseMatrix  deriv,
  Epetra_SerialDenseMatrix&       metrictensor,
  double                         *sqrtdetg)
{
  /*
  |                                              0 1 2
  |                                             +-+-+-+
  |       0 1 2              0...iel-1          | | | | 0
  |      +-+-+-+             +-+-+-+-+          +-+-+-+
  |      | | | | 1           | | | | | 0        | | | | .
  |      +-+-+-+       =     +-+-+-+-+       *  +-+-+-+ .
  |      | | | | 2           | | | | | 1        | | | | .
  |      +-+-+-+             +-+-+-+-+          +-+-+-+
  |                                             | | | | iel-1
  |		     	      	     	        +-+-+-+
  |
  |       dxyzdrs             deriv              xyze^T
  |
  |
  |                                     +-            -+
  |  	   	    	    	        | dx   dy   dz |
  |  	   	    	    	        | --   --   -- |
  | 	   	   	   	        | dr   dr   dr |
  | 	yields               dxyzdrs =  |              |
  |  	   	    	    	        | dx   dy   dz |
  |  	   	    	    	        | --   --   -- |
  | 	   	   	   	        | ds   ds   ds |
  |                                     +-            -+
  |
  */
  Epetra_SerialDenseMatrix dxyzdrs (2,3);

  dxyzdrs.Multiply('N','T',1.0,deriv,xyze,0.0);

  /*
  |
  |      +-           -+    +-            -+   +-            -+ T
  |      |             |    | dx   dy   dz |   | dx   dy   dz |
  |      |  g11   g12  |    | --   --   -- |   | --   --   -- |
  |      |             |    | dr   dr   dr |   | dr   dr   dr |
  |      |             |  = |              | * |              |
  |      |             |    | dx   dy   dz |   | dx   dy   dz |
  |      |  g21   g22  |    | --   --   -- |   | --   --   -- |
  |      |             |    | ds   ds   ds |   | ds   ds   ds |
  |      +-           -+    +-            -+   +-            -+
  |
  | the calculation of g21 is redundant since g21=g12
  */
  metrictensor.Multiply('N','T',1.0,dxyzdrs,dxyzdrs,0.0);

/*
                          +--------------+
                         /               |
           sqrtdetg =   /  g11*g22-g12^2
                      \/
*/

  sqrtdetg[0]= sqrt(metrictensor(0,0)*metrictensor(1,1)
                    -
                    metrictensor(0,1)*metrictensor(1,0));

  return;
}

/*----------------------------------------------------------------------*
 |  Integrate shapefunctions over surface (private)            gjb 07/07|
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Fluid3Surface::IntegrateShapeFunction(ParameterList& params,
                  DRT::Discretization&       discretization,
                  vector<int>&               lm,
                  Epetra_SerialDenseVector&  elevec1,
                  const std::vector<double>& edispnp)
{
  // there are 3 velocities and 1 pressure
  const int numdf = 4;

  const DiscretizationType distype = this->Shape();

  // set number of nodes
  const int iel   = this->NumNode();

  GaussRule2D  gaussrule = intrule2D_undefined;
  switch(distype)
  {
  case quad4:
      gaussrule = intrule_quad_4point;
      break;
  case quad8: case quad9:
      gaussrule = intrule_quad_9point;
      break;
  case tri3 :
      gaussrule = intrule_tri_3point;
      break;
  case tri6:
      gaussrule = intrule_tri_6point;
      break;
  default:
      dserror("shape type unknown!\n");
  }

    // allocate vector for shape functions and matrix for derivatives
  Epetra_SerialDenseVector      funct       (iel);
  Epetra_SerialDenseMatrix      deriv       (2,iel);

  // node coordinates
  Epetra_SerialDenseMatrix      xyze        (3,iel);

  // the metric tensor and the area of an infintesimal surface element
  Epetra_SerialDenseMatrix      metrictensor(2,2);
  double                        drs;

  // get node coordinates
  for(int i=0;i<iel;i++)
  {
    xyze(0,i)=this->Nodes()[i]->X()[0];
    xyze(1,i)=this->Nodes()[i]->X()[1];
    xyze(2,i)=this->Nodes()[i]->X()[2];
  }

  if (parent_->IsAle())
  {
    dsassert(edispnp.size()!=0,"paranoid");

    for (int i=0;i<iel;i++)
    {
      xyze(0,i) += edispnp[4*i];
      xyze(1,i) += edispnp[4*i+1];
      xyze(2,i) += edispnp[4*i+2];
    }
  }

  /*----------------------------------------------------------------------*
  |               start loop over integration points                     |
  *----------------------------------------------------------------------*/
  const IntegrationPoints2D  intpoints = getIntegrationPoints2D(gaussrule);

  for (int gpid=0; gpid<intpoints.nquad; gpid++)
  {
    const double e0 = intpoints.qxg[gpid][0];
    const double e1 = intpoints.qxg[gpid][1];

    // get shape functions and derivatives in the plane of the element
    shape_function_2D(funct, e0, e1, distype);
    shape_function_2D_deriv1(deriv, e0, e1, distype);

    // compute measure tensor for surface element and the infinitesimal
    // area element drs for the integration

    f3_metric_tensor_for_surface(xyze,deriv,metrictensor,&drs);

    // values are multiplied by the product from inf. area element,
    // the gauss weight

    const double fac = intpoints.qwgt[gpid] * drs;

    for (int node=0;node<iel;++node)
    {
      for(int dim=0;dim<3;dim++)
      {
        elevec1[node*numdf+dim]+= funct[node] * fac;
      }
    }

  } /* end of loop over integration points gpid */


return;
} // DRT::ELEMENTS::Fluid3Surface::IntegrateShapeFunction


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Fluid3Surface::ElementNodeNormal(ParameterList& params,
                                                     DRT::Discretization&       discretization,
                                                     vector<int>&               lm,
                                                     Epetra_SerialDenseVector&  elevec1,
                                                     const std::vector<double>& edispnp)
{
  // there are 3 velocities and 1 pressure
  const int numdf = 4;

  const DiscretizationType distype = this->Shape();

  // set number of nodes
  const int iel   = this->NumNode();

  GaussRule2D  gaussrule = intrule2D_undefined;
  switch(distype)
  {
  case quad4:
      gaussrule = intrule_quad_4point;
      break;
  case quad8: case quad9:
      gaussrule = intrule_quad_9point;
      break;
  case tri3 :
      gaussrule = intrule_tri_3point;
      break;
  case tri6:
      gaussrule = intrule_tri_6point;
      break;
  default:
      dserror("shape type unknown!\n");
  }

    // allocate vector for shape functions and matrix for derivatives
  Epetra_SerialDenseVector      funct       (iel);
  Epetra_SerialDenseMatrix 	deriv       (2,iel);

  // node coordinates
  Epetra_SerialDenseMatrix      xyze        (3,iel);

  // the metric tensor and the area of an infintesimal surface element
  Epetra_SerialDenseMatrix 	metrictensor(2,2);
  double                        drs;

  // get node coordinates
  for(int i=0;i<iel;i++)
  {
    xyze(0,i)=this->Nodes()[i]->X()[0];
    xyze(1,i)=this->Nodes()[i]->X()[1];
    xyze(2,i)=this->Nodes()[i]->X()[2];
  }

  if (parent_->IsAle())
  {
    dsassert(edispnp.size()!=0,"paranoid");

    for (int i=0;i<iel;i++)
    {
      xyze(0,i) += edispnp[4*i];
      xyze(1,i) += edispnp[4*i+1];
      xyze(2,i) += edispnp[4*i+2];
    }
  }

  /*----------------------------------------------------------------------*
  |               start loop over integration points                     |
  *----------------------------------------------------------------------*/
  const IntegrationPoints2D  intpoints = getIntegrationPoints2D(gaussrule);

  for (int gpid=0; gpid<intpoints.nquad; gpid++)
  {
    const double e0 = intpoints.qxg[gpid][0];
    const double e1 = intpoints.qxg[gpid][1];

    // get shape functions and derivatives in the plane of the element
    shape_function_2D(funct, e0, e1, distype);
    shape_function_2D_deriv1(deriv, e0, e1, distype);

    // compute measure tensor for surface element and the infinitesimal
    // area element drs for the integration

    f3_metric_tensor_for_surface(xyze,deriv,metrictensor,&drs);

    // values are multiplied by the product from inf. area element,
    // the gauss weight and the constant
    // belonging to the time integration algorithm (theta*dt for
    // one step theta, 2/3 for bdf with dt const.)

    //const double fac = intpoints.qwgt[gpid] * drs * thsl;
    const double fac = intpoints.qwgt[gpid] * drs;

    //this element's normal vector
    Epetra_SerialDenseVector   norm(numdf);
    double length = 0.0;
    norm[0] = (xyze(1,1)-xyze(1,0))*(xyze(2,2)-xyze(2,0))+(xyze(2,1)-xyze(2,0))*(xyze(1,2)-xyze(1,0));
    norm[1] = (xyze(2,1)-xyze(2,0))*(xyze(0,2)-xyze(0,0))+(xyze(0,1)-xyze(0,0))*(xyze(2,2)-xyze(2,0));
    norm[2] = (xyze(0,1)-xyze(0,0))*(xyze(1,2)-xyze(1,0))+(xyze(1,1)-xyze(1,0))*(xyze(0,2)-xyze(0,0));

    length = sqrt(norm[0]*norm[0]+norm[1]*norm[1]+norm[2]*norm[2]);

    norm[0] = (1.0/length)*norm[0];
    norm[1] = (1.0/length)*norm[1];
    norm[2] = (1.0/length)*norm[2];

    for (int node=0;node<iel;++node)
    {
      for(int dim=0;dim<3;dim++)
      {
        elevec1[node*numdf+dim]+=
          funct[node] * fac * norm[dim];
      }
    }
  } /* end of loop over integration points gpid */
} // DRT::ELEMENTS::Fluid3Surface::ElementNodeNormal


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Fluid3Surface::FlowRateParameterCaculation(ParameterList& params,
                  DRT::Discretization&       discretization,
                  vector<int>&               lm,
                  Epetra_SerialDenseVector&  elevec1)
{
  const int iel   = this->NumNode();
  const DiscretizationType distype = this->Shape();
  // allocate vector for shape functions and matrix for derivatives
  Epetra_SerialDenseVector  	funct       (iel);
  Epetra_SerialDenseMatrix  	deriv       (2,iel);

  // node coordinates
  Epetra_SerialDenseMatrix  	xyze        (3,iel);

  // the metric tensor and the area of an infintesimal surface element
  Epetra_SerialDenseMatrix 	metrictensor  (2,2);
  double                        drs;

  GaussRule2D  gaussrule = intrule2D_undefined;
  switch(distype)
  {
  	case quad4:
  	gaussrule = intrule_quad_4point;
	break;
	case quad8: case quad9:
	gaussrule = intrule_quad_9point;
	break;
	case tri3 :
	gaussrule = intrule_tri_3point;
	break;
	case tri6:
	gaussrule = intrule_tri_6point;
	break;
	default:
	dserror("shape type unknown!\n");
  }

  RefCountPtr<const Epetra_Vector> velnp = discretization.GetState("velnp");

  if (velnp==null){
  	dserror("Cannot get state vector 'velnp'");
  }

  // extract local values from the global vectors
  vector<double> myvelnp(lm.size());
  DRT::UTILS::ExtractMyValues(*velnp,myvelnp,lm);

  double flowrate    = params.get<double>("Outlet flowrate");
  double area        = params.get<double>("Area calculation");

  // create blitz objects for element arrays
  const int numnode = NumNode();
  blitz::Array<double, 1> eprenp(numnode);
  blitz::Array<double, 2> evelnp(3,numnode,blitz::ColumnMajorArray<2>());
  blitz::Array<double, 2> evhist(3,numnode,blitz::ColumnMajorArray<2>());

  // split velocity and pressure, insert into element arrays
  for (int i=0;i<numnode;++i)
  {
    evelnp(0,i) = myvelnp[0+(i*4)];
    evelnp(1,i) = myvelnp[1+(i*4)];
    evelnp(2,i) = myvelnp[2+(i*4)];
    eprenp(i) = myvelnp[3+(i*4)];
  }

  for(int i=0;i<iel;i++)
  {
    xyze(0,i)=this->Nodes()[i]->X()[0];
    xyze(1,i)=this->Nodes()[i]->X()[1];
    xyze(2,i)=this->Nodes()[i]->X()[2];
  }

  const IntegrationPoints2D  intpoints = getIntegrationPoints2D(gaussrule);
  for (int gpid=0; gpid<intpoints.nquad; gpid++)
  {
    const double e0 = intpoints.qxg[gpid][0];
    const double e1 = intpoints.qxg[gpid][1];

    // get shape functions and derivatives in the plane of the element
    shape_function_2D(funct, e0, e1, distype);
    shape_function_2D_deriv1(deriv, e0, e1, distype);

    //Calculate infinitesimal area of element (drs)
    f3_metric_tensor_for_surface(xyze,deriv,metrictensor,&drs);

    if (iel==4)
      drs=drs*4;
    else
      drs=drs/2;

    //Compute elment flowrate
    for (int node=0;node<iel;++node)
    {
      for(int dim=0;dim<3;dim++)
      {
	flowrate += funct[node]*evelnp(dim,node)*drs;
      }
    }
    area += drs;
  }

  params.set<double>("Area calculation", area);
  params.set<double>("Outlet flowrate", flowrate);
}


  /*----------------------------------------------------------------------*
  |  Impedance related parameters on boundary elements          AC 03/08  |
  *----------------------------------------------------------------------*/
void DRT::ELEMENTS::Fluid3Surface::ImpedanceIntegration(ParameterList& params,
                  DRT::Discretization&       discretization,
                  vector<int>&               lm,
                  Epetra_SerialDenseVector&  elevec1)
{
  const int iel   = this->NumNode();
  const DiscretizationType distype = this->Shape();
  const int numdf = 4;
  const double thsl = params.get("thsl",0.0);

  double density; // density of my parent element

  // get material of volume element this surface belongs to
  RefCountPtr<MAT::Material> mat = parent_->Material();

  if( mat->MaterialType()    != m_carreauyasuda
      && mat->MaterialType() != m_modpowerlaw
      && mat->MaterialType() != m_fluid)
          dserror("Material law is not a fluid");

  MATERIAL* actmat = NULL;

  if(mat->MaterialType()== m_fluid)
  {
    actmat = static_cast<MAT::NewtonianFluid*>(mat.get())->MaterialData();
    density = actmat->m.fluid->density;
  }
  else if(mat->MaterialType()== m_carreauyasuda)
  {
    actmat = static_cast<MAT::CarreauYasuda*>(mat.get())->MaterialData();
    density = actmat->m.carreauyasuda->density;
  }
  else if(mat->MaterialType()== m_modpowerlaw)
  {
    actmat = static_cast<MAT::ModPowerLaw*>(mat.get())->MaterialData();
    density = actmat->m.modpowerlaw->density;
  }
  else
    dserror("fluid material expected but got type %d", mat->MaterialType());

  double invdensity = 1.0/density; // we shall need the inverse of rho


  // allocate vector for shape functions and matrix for derivatives
  Epetra_SerialDenseVector  	funct       (iel);
  Epetra_SerialDenseMatrix  	deriv       (2,iel);

  // node coordinates
  Epetra_SerialDenseMatrix  	xyze        (3,iel);

  // the metric tensor and the area of an infintesimal surface element
  Epetra_SerialDenseMatrix 	metrictensor  (2,2);
  double                      drs;

  // pressure from time integration
  double pressure = params.get<double>("ConvolutedPressure");

  GaussRule2D  gaussrule = intrule2D_undefined;
  switch(distype)
  {
  	case quad4:
    gaussrule = intrule_quad_4point;
    break;
    case quad8: case quad9:
    gaussrule = intrule_quad_9point;
    break;
    case tri3 :
    gaussrule = intrule_tri_3point;
    break;
    case tri6:
    gaussrule = intrule_tri_6point;
    break;
    default:
    dserror("shape type unknown!\n");
  }

  for(int i=0;i<iel;i++)
  {
  	xyze(0,i)=this->Nodes()[i]->X()[0];
    xyze(1,i)=this->Nodes()[i]->X()[1];
    xyze(2,i)=this->Nodes()[i]->X()[2];
  }

  const IntegrationPoints2D  intpoints = getIntegrationPoints2D(gaussrule);
  for (int gpid=0; gpid<intpoints.nquad; gpid++)
  {
    const double e0 = intpoints.qxg[gpid][0];
    const double e1 = intpoints.qxg[gpid][1];

    // get shape functions and derivatives in the plane of the element
    shape_function_2D(funct, e0, e1, distype);
    shape_function_2D_deriv1(deriv, e0, e1, distype);

  	//Calculate infinitesimal area of element (drs)
  	f3_metric_tensor_for_surface(xyze,deriv,metrictensor,&drs);
  	//Calculate streamwise velocity gradient

    Epetra_SerialDenseMatrix  	DistanceVector       (3,2);
    double 	Magnitude;
    //Get two inplane vectors
    DistanceVector(0,0)=xyze(0,1)-xyze(0,0);
    DistanceVector(1,0)=xyze(1,1)-xyze(1,0);
    DistanceVector(2,0)=xyze(2,1)-xyze(2,0);

    DistanceVector(0,1)=xyze(0,2)-xyze(0,0);
    DistanceVector(1,1)=xyze(1,2)-xyze(1,0);
    DistanceVector(2,1)=xyze(2,2)-xyze(2,0);

   //Calculate normal coordinate
    Epetra_SerialDenseMatrix	SurfaceNormal		(3,1);

    SurfaceNormal(0,0)=DistanceVector(1,0)*DistanceVector(2,1)-DistanceVector(2,0)*DistanceVector(1,1);
    SurfaceNormal(1,0)=DistanceVector(2,0)*DistanceVector(0,1)-DistanceVector(0,0)*DistanceVector(2,1);
	SurfaceNormal(2,0)=DistanceVector(0,0)*DistanceVector(1,1)-DistanceVector(1,0)*DistanceVector(0,1);
    Magnitude=sqrt(pow(SurfaceNormal(0,0),2)+pow(SurfaceNormal(1,0),2)+pow(SurfaceNormal(2,0),2));
    SurfaceNormal(0,0)=SurfaceNormal(0,0)/Magnitude;
    SurfaceNormal(1,0)=SurfaceNormal(1,0)/Magnitude;
    SurfaceNormal(2,0)=SurfaceNormal(2,0)/Magnitude;

    const double fac = intpoints.qwgt[gpid] * drs * thsl * pressure * invdensity;

    for (int node=0;node<iel;++node)
    {
    	for(int dim=0;dim<3;dim++)
    	{
    		elevec1[node*numdf+dim]+=
    		funct[node] * fac * SurfaceNormal(dim,0);
        }
    }

  }
  return;
}//DRT::ELEMENTS::Fluid3Surface::FlowRateParameterCaculation

#endif  // #ifdef CCADISCRET
#endif // #ifdef D_FLUID3
