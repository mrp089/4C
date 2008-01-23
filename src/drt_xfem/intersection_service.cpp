/*!----------------------------------------------------------------------
\file intersection_service.cpp

\brief collection of math tools for the interface determination of trv1o meshes

    ML      math library for the interface computation
 
    
<pre>
Maintainer: Ursula Mayer
</pre>
*----------------------------------------------------------------------*/

#ifdef CCADISCRET

#include "intersection_service.H"
#include "intersection_math.H"
#include "../drt_lib/drt_discret.H"
#include "../drt_lib/drt_utils_fem_shapefunctions.H"
#include "../drt_lib/drt_utils_local_connectivity_matrices.H"
#include "../drt_lib/drt_element.H"

using namespace XFEM;
using namespace DRT::UTILS;


static double           sqrarg;
#define                 SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)
//inline double SQR(double sqrarg)
//{
//    return ((sqrarg) == 0.0 ? 0.0 : sqrarg*sqrarg);
//}


/*----------------------------------------------------------------------*
 |  ML:     adds two Epetra_SerialDenseVector                u.may 06/07|
 *----------------------------------------------------------------------*/
Epetra_SerialDenseVector XFEM::addTwoVectors(   
    const Epetra_SerialDenseVector&   v1,
    const Epetra_SerialDenseVector&   v2)
{   
    Epetra_SerialDenseVector vResult(v1.Length());
    
    dsassert(v1.Length() == v2.Length(), "both vectors need to have the same size\n"); 

    for(int i = 0; i < v1.Length(); i++)
        vResult[i] = v1[i] + v2[i];
 
    return vResult;
}
    
    
   
/*----------------------------------------------------------------------*
 |  ML:     adds two vector<double>                          u.may 06/07|
 *----------------------------------------------------------------------*/
vector<double> XFEM::addTwoVectors(
    const vector<double>&   v1,
    const vector<double>&   v2)
{   
    vector<double> vResult(v1.size());
    
    dsassert(v1.size() == v2.size(), "both vectors need to have the same size\n");

    for(unsigned int i = 0; i < v1.size(); i++)
        vResult[i] = v1[i] + v2[i];
 
    return vResult;
}
    
    

/*----------------------------------------------------------------------*
 |  ML:     subtracts one Epetra_SerialDenseVector from   u.may 06/07   |
 |          another Epetra_SerialDenseVector.                           |
 |          The result is stored in v1                                  |
 *----------------------------------------------------------------------*/
Epetra_SerialDenseVector XFEM::subtractsTwoVectors( 
    const Epetra_SerialDenseVector& v1,
    const Epetra_SerialDenseVector& v2)
{   
    
    Epetra_SerialDenseVector vResult(v1.Length());
    
    if(v1.Length() != v2.Length())
        dserror("both vectors need to have the same size\n"); 

    for(int i = 0; i < v1.Length(); i++)
        vResult[i] = v1[i] - v2[i];
 
    return vResult;
}

    

/*----------------------------------------------------------------------*
 |  ML :    subtracts one vector<double> from another        u.may 06/07|
 |          vector<double> . The result is stored in v1.                |
 *----------------------------------------------------------------------*/
vector<double> XFEM::subtractsTwoVectors(   
    const vector <double>& v1,
    const vector <double>& v2)
{   
    vector <double>  vResult(v1.size());
    
    if(v1.size() != v2.size())
        dserror("both vectors need to have the same size\n"); 

    for(unsigned int i = 0; i < v1.size(); i++)
        vResult[i] = v1[i] - v2[i];
 
    return vResult;
}



/*----------------------------------------------------------------------*
 |  ML:     computes the cross product                       u.may 08/07|
 |          of 2 Epetra_SerialDenseVector c = a x b                     |
 *----------------------------------------------------------------------*/  
Epetra_SerialDenseVector XFEM::computeCrossProduct(
    const Epetra_SerialDenseVector& a,
    const Epetra_SerialDenseVector& b)
{
    Epetra_SerialDenseVector c(3);
   
    c[0] = a[1]*b[2] - a[2]*b[1];
    c[1] = a[2]*b[0] - a[0]*b[2];
    c[2] = a[0]*b[1] - a[1]*b[0];
    
    return c;
}



/*----------------------------------------------------------------------*
 |  ML:     normalizes a Epetra_SerialDenseVector            u.may 08/07|
 *----------------------------------------------------------------------*/  
void XFEM::normalizeVector(   
    Epetra_SerialDenseVector&     v)
{
    const double norm = v.Norm2();
    v.Scale(1.0/norm);
    return;
}



/*----------------------------------------------------------------------*
 |    theorem of pythagoras                                 u.may 09/07 |
 |    computes ( a^2 + b^2 ) ^ (1/2)                                    |
 |    (modified from NUMERICAL RECIPES)                                 |
 *----------------------------------------------------------------------*/  
double XFEM::pythagoras(
    const double  a, 
    const double  b)
{
    const double absa=fabs(a);
    const double absb=fabs(b);
    if (absa > absb) return absa*sqrt(1.0+SQR(absb/absa));
    else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+SQR(absa/absb)));
}


/*----------------------------------------------------------------------*
 |  GM:     checks if a certain element is a                 u.may 06/07|
 |          volume  element with help of the discretization type        |
 *----------------------------------------------------------------------*/  
bool XFEM::checkIfVolumeElement(
        const DRT::Element::DiscretizationType distype)
{
    bool isVolume = false;
    
    if( distype == DRT::Element::hex8  ||
        distype == DRT::Element::hex20 ||
        distype == DRT::Element::hex27 ||
        distype == DRT::Element::tet4  ||
        distype == DRT::Element::tet10  )
    {
        isVolume = true;        
    }
    return isVolume;
}


/*----------------------------------------------------------------------*
 |  GM:     checks if a certain element is a                 u.may 06/07|
 |          with help of the discretization type                        |
 *----------------------------------------------------------------------*/  
bool XFEM::checkIfSurfaceElement(
        const DRT::Element::DiscretizationType distype)
{
    bool isSurface = false;
    
    if( distype == DRT::Element::quad4 ||
        distype == DRT::Element::quad8 ||
        distype == DRT::Element::quad9 ||
        distype == DRT::Element::tri3  ||
        distype == DRT::Element::tri6  )
    {
        isSurface = true;       
    }
    return isSurface;
}


/*----------------------------------------------------------------------*
 |  GM:     checks if a certain element is a                 u.may 06/07|
 |          line element with help of the discretization type           |
 *----------------------------------------------------------------------*/  
bool XFEM::checkIfLineElement(
        const DRT::Element::DiscretizationType distype)
{
    bool isLine = false;
    
    if( distype == DRT::Element::line2 ||
        distype == DRT::Element::line3 )
    {
        isLine = true;      
    }
    return isLine;
}


/*----------------------------------------------------------------------*
 |  ICS:    checks if a position is within an XAABB          u.may 06/07|
 *----------------------------------------------------------------------*/
bool XFEM::isPositionWithinXAABB(    
    const Epetra_SerialDenseVector&    pos,
    const Epetra_SerialDenseMatrix&    XAABB)
{
    const int nsd = 3;
    bool isWithin = true;
    for (int isd=0; isd<nsd; isd++)
    {
        const double diffMin = XAABB(isd,0) - TOL7;
        const double diffMax = XAABB(isd,1) + TOL7;
        
       // printf("nodal value =  %f, min =  %f, max =  %f\n", node[dim], diffMin, diffMax);
        
        if((pos(isd) < diffMin)||(pos(isd) > diffMax)) //check again !!!!!   
        {
            isWithin = false;
            break;
        }
    }
   
    return isWithin;
}


/*----------------------------------------------------------------------*
 |  ICS:    checks if a pos is within an XAABB               u.may 06/07|
 *----------------------------------------------------------------------*/
bool XFEM::isLineWithinXAABB(    
    const Epetra_SerialDenseVector&    pos1,
    const Epetra_SerialDenseVector&    pos2,
    const Epetra_SerialDenseMatrix&    XAABB)
{
    const int nsd = 3;
    bool isWithin = true;
    int isd = -1;
    
    for(isd=0; isd<nsd; isd++)
        if(fabs(pos1[isd]-pos2[isd]) > TOL7)
            break;
    
    for(int ksd = 0; ksd < nsd; ksd++)
    {
        if(ksd != isd)
        {
            double min = XAABB(ksd,0) - TOL7;
            double max = XAABB(ksd,1) + TOL7;
   
            if((pos1[ksd] < min)||(pos1[ksd] > max))
                isWithin = false;
            
        }
        if(!isWithin)
            break;
    }
        
    if(isWithin && isd > -1)
    {
        isWithin = false;
        const double min = XAABB(isd,0) - TOL7;
        const double max = XAABB(isd,1) + TOL7;
                            
        if( ((pos1[isd] < min) && (pos2[isd] > max)) ||  
            ((pos2[isd] < min) && (pos1[isd] > max)) )
            isWithin = true;
    }
    return isWithin;
}


/*----------------------------------------------------------------------*
 |  CLI:    checks if a position is within a given element   u.may 06/07|   
 *----------------------------------------------------------------------*/
bool XFEM::checkPositionWithinElement(  
    DRT::Element*                       element,
    const Epetra_SerialDenseVector&     x)
{

    bool nodeWithinElement = true;
    int iter = 0;
    const int dim = getDimension(element);
    const int maxiter = 20;
    double residual = 1.0;
    
    Epetra_SerialDenseMatrix A(dim,dim);
    Epetra_SerialDenseVector b(dim);
    Epetra_SerialDenseVector dx(dim);
    Epetra_SerialDenseVector xsi(dim);
            
    updateRHSForNWE( dim, b, xsi, x, element);
   
    while(residual > TOL14)
    {   
        updateAForNWE( dim, A, xsi, element);
   
        if(!gaussElimination(A, b, dx, true, dim, 1))
        {
            nodeWithinElement = false;
            break;
        }   
        
        xsi = addTwoVectors(xsi,dx);
        updateRHSForNWE(dim, b, xsi, x, element);
        residual = b.Norm2();
        iter++; 
        
        if(iter >= maxiter)
        {   
            nodeWithinElement = false;
            break;
        }   
    }
    
    //printf("iter = %d\n", iter);
    //printf("xsi0 = %20.16f\t, xsi1 = %20.16f\t, xsi2 = %20.16f\t, res = %20.16f\t, tol = %20.16f\n", xsi[0],xsi[1],xsi[2], residual, TOL14);
    
    for(int i=0; i<dim; i++)
        if( (fabs(xsi[i])-1.0) > TOL7)     
        {    
            nodeWithinElement = false;
            break;
        }
        
    return nodeWithinElement;
}


/*----------------------------------------------------------------------*
 |  CLI:    checks if a node is within a given element       u.may 06/07|   
 *----------------------------------------------------------------------*/
bool XFEM::checkPositionWithinElement(  
    DRT::Element*                       element,
    const Epetra_SerialDenseVector&     x,
    Epetra_SerialDenseVector&           xsi)
{

    bool nodeWithinElement = true;
    int iter = 0;
    const int dim = getDimension(element);
    const int maxiter = 20;
    double residual = 1.0;
    
    Epetra_SerialDenseMatrix A(dim,dim);
    Epetra_SerialDenseVector b(dim);
    Epetra_SerialDenseVector dx(dim);
    
    xsi.Scale(0.0);
            
    updateRHSForNWE( dim, b, xsi, x, element);
   
    while(residual > TOL14)
    {   
        updateAForNWE( dim, A, xsi, element);
   
        if(!gaussElimination(A, b, dx, true, dim, 1))
        {
            nodeWithinElement = false;
            break;
        }   
        
        xsi = addTwoVectors(xsi,dx);
        updateRHSForNWE(dim, b, xsi, x, element);
        residual = b.Norm2();
        iter++; 
        
        if(iter >= maxiter)
        {   
            nodeWithinElement = false;
            break;
        }   
    }
    
    //printf("iter = %d\n", iter);
    //printf("xsi0 = %20.16f\t, xsi1 = %20.16f\t, xsi2 = %20.16f\t, res = %20.16f\t, tol = %20.16f\n", xsi[0],xsi[1],xsi[2], residual, TOL14);
    
    for(int i=0; i<dim; i++)
        if( (fabs(xsi[i])-1.0) > TOL7)     
        {    
            nodeWithinElement = false;
            break;
        }
        
    return nodeWithinElement;
}


/*----------------------------------------------------------------------*
 |  CLI:    checks if a position is within a given mesh      a.ger 12/07|   
 *----------------------------------------------------------------------*/
bool XFEM::PositionWithinDiscretization(  
    RCP<DRT::Discretization>            dis,
    const Epetra_SerialDenseVector&     x)
{
    bool nodeWithinMesh = false;
    
    // loop all elements on this processor
    for (int i=0; i<dis->NumMyRowElements(); ++i)
    {
        DRT::Element* ele = dis->lRowElement(i);
        if (isPositionWithinXAABB(x, computeFastXAABB(ele)))
        {
            nodeWithinMesh = checkPositionWithinElement(ele, x);
            if (nodeWithinMesh)
                break;
        }
    }

    // TODO: in parallel, we have to ask all processors, whether there is any match!!!!
#ifdef PARALLEL
    dserror("not implemented, yet");
#endif
    return nodeWithinMesh;
}

/*----------------------------------------------------------------------*
 |  CLI:    checks if a position is within condition-enclosed region      a.ger 12/07|   
 *----------------------------------------------------------------------*/
bool XFEM::PositionWithinCondition(
        const blitz::Array<double,1>&       x_in,
        const int                           xfem_condition_label, 
        RCP<DRT::Discretization>            cutterdis
    )
{
    const int nsd = 3;
    Epetra_SerialDenseVector x(nsd);
    for (int isd = 0; isd < nsd; ++isd) {
        x(isd) = x_in(isd);
    }

    // TODO: use label to identify the surface/xfem condition
    
    bool nodeWithinMesh = false;
    // loop all elements on this processor
    for (int i=0; i<cutterdis->NumMyRowElements(); ++i)
    {
        DRT::Element* ele = cutterdis->lRowElement(i);
        if (isPositionWithinXAABB(x, computeFastXAABB(ele)))
        {
            nodeWithinMesh = checkPositionWithinElement(ele, x);
            if (nodeWithinMesh)
                break;
        }
    }

    // TODO: in parallel, we have to ask all processors, whether there is any match!!!!
#ifdef PARALLEL
    dserror("not implemented, yet");
#endif
    return nodeWithinMesh;
}


/*----------------------------------------------------------------------*
 |  CLI:    updates the Jacobi matrix for the computation    u.may 06/07|
 |          if a node is in a given element                             |
 *----------------------------------------------------------------------*/
void XFEM::updateAForNWE(   
    const int                   dim,
    Epetra_SerialDenseMatrix&   A,
    Epetra_SerialDenseVector&   xsi,
    DRT::Element*               element)                                                  
{   
    const int numNodes = element->NumNode();
    blitz::Array<double,2> deriv1(dim, numNodes, blitz::ColumnMajorArray<2>());
    
    A.Scale(0.0);
   
    switch(dim)
    {
        case 1:
        {
            shape_function_1D_deriv1(deriv1, xsi[0], element->Shape());
            break;
        }
        case 2:
        {
            shape_function_2D_deriv1(deriv1, xsi[0], xsi[1], element->Shape());
            break;
        }
        case 3:
        {
            shape_function_3D_deriv1(deriv1, xsi[0], xsi[1], xsi[2], element->Shape());
            break;
        }
        default:
            dserror("dimension of the element is not correct");
    }
    
    for(int inode=0; inode<numNodes; inode++) 
    {
        const double* x = element->Nodes()[inode]->X();
        for(int isd=0; isd<dim; ++isd)
        {
            const double nodalCoord = x[isd];
            for(int jsd=0; jsd<dim; ++jsd)
                A[isd][jsd] += nodalCoord * deriv1(jsd,inode);
        }
    }      
}


/*----------------------------------------------------------------------*
 |  CLI:    updates the rhs for the computation if a         u.may 06/07|
 |          node is in a given element                                  |
 *----------------------------------------------------------------------*/
void XFEM::updateRHSForNWE( 
    const int                           dim,
    Epetra_SerialDenseVector&           b,
    Epetra_SerialDenseVector&           xsi,
    const Epetra_SerialDenseVector&     x,
    DRT::Element*                       element)                                                  
{
    const int numNodes = element->NumNode();
    blitz::Array<double,1> funct(numNodes);
      
    b.Scale(0.0);
     
    switch(dim)
    {
        case 1:
        {
            shape_function_1D(funct, xsi[0], element->Shape());
            break;
        }
        case 2:
        {
            shape_function_2D(funct, xsi[0], xsi[1], element->Shape());
            break;
        }
        case 3:
        {
            shape_function_3D(funct, xsi[0], xsi[1], xsi[2], element->Shape());
            break;
        }
        default:
            dserror("dimension of the element is not correct");
    }
    
    for(int j=0; j<numNodes; j++)
    {
        const double* x = element->Nodes()[j]->X();
        for(int i=0; i<dim; i++)
            b[i] -= x[i] * funct(j);
    }
      
    for(int i=0; i<dim; i++)
        b[i] += x[i];
}


/*----------------------------------------------------------------------*
 |  ICS:    computes an extended axis-aligned bounding box   u.may 06/07|
 |          XAABB for a given element                                   |
 *----------------------------------------------------------------------*/
Epetra_SerialDenseMatrix XFEM::computeFastXAABB( 
    DRT::Element* element)
{
    double  maxDistance;
    const int nsd = 3;
    Epetra_SerialDenseMatrix XAABB(nsd, 2);
    
    DRT::Node* node = element->Nodes()[0];
    for(int dim=0; dim<nsd; dim++)
    {
        XAABB(dim, 0) = node->X()[dim] - TOL7;
        XAABB(dim, 1) = node->X()[dim] + TOL7;
    }
    
    for(int i=1; i<element->NumNode(); i++)
    {
        DRT::Node* nodeEle = element->Nodes()[i];
        for(int dim=0; dim<nsd; dim++)
        {
            XAABB(dim, 0) = std::min( XAABB(dim, 0), nodeEle->X()[dim] - TOL7);
            XAABB(dim, 1) = std::max( XAABB(dim, 1), nodeEle->X()[dim] + TOL7);
        }
    }
    
    maxDistance = fabs(XAABB(0,1) - XAABB(0,0));
    for(int dim=1; dim<nsd; dim++)
       maxDistance = std::max(maxDistance, fabs(XAABB(dim,1)-XAABB(dim,0)) );
    
    // subtracts half of the maximal distance to minX, minY, minZ
    // adds half of the maximal distance to maxX, maxY, maxZ 
    for(int dim=0; dim<nsd; dim++)
    {
        XAABB(dim, 0) = XAABB(dim, 0) - 0.5*maxDistance;
        XAABB(dim, 1) = XAABB(dim, 1) + 0.5*maxDistance;
    }   
    
    /*
    printf("\n");
    printf("axis-aligned bounding box:\n minX = %f\n minY = %f\n minZ = %f\n maxX = %f\n maxY = %f\n maxZ = %f\n", 
              XAABB(0,0), XAABB(1,0), XAABB(2,0), XAABB(0,1), XAABB(1,1), XAABB(2,1));
    printf("\n");
    */
    
    return XAABB;
}


/*----------------------------------------------------------------------*
 |  ICS:    checks if two XAABB's intersect                  u.may 06/07|
 *----------------------------------------------------------------------*/
bool XFEM::intersectionOfXAABB(  
    const Epetra_SerialDenseMatrix&     cutterXAABB, 
    const Epetra_SerialDenseMatrix&     xfemXAABB)
{
    
  /*====================================================================*/
  /* bounding box topology*/
  /*--------------------------------------------------------------------*/
  /* parameter coordinates (x,y,z) of nodes
   * node 0: (minX, minY, minZ)
   * node 1: (maxX, minY, minZ)
   * node 2: (maxX, maxY, minZ)
   * node 3: (minX, maxY, minZ)
   * node 4: (minX, minY, maxZ)
   * node 5: (maxX, minY, maxZ)
   * node 6: (maxX, maxY, maxZ)
   * node 7: (minX, maxY, maxZ)
   * 
   *                      z
   *                      |           
   *             4========|================7
   *           //|        |               /||
   *          // |        |              //||
   *         //  |        |             // ||
   *        //   |        |            //  ||
   *       //    |        |           //   ||
   *      //     |        |          //    ||
   *     //      |        |         //     ||
   *     5=========================6       ||
   *    ||       |        |        ||      ||
   *    ||       |        o--------||---------y
   *    ||       |       /         ||      ||
   *    ||       0------/----------||------3
   *    ||      /      /           ||     //
   *    ||     /      /            ||    //
   *    ||    /      /             ||   //
   *    ||   /      /              ||  //
   *    ||  /      /               || //
   *    || /      x                ||//
   *    ||/                        ||/
   *     1=========================2
   *
   */
  /*====================================================================*/
    
    bool intersection =  false;
    std::vector < Epetra_SerialDenseVector > nodes(8, Epetra_SerialDenseVector(3));
    
    nodes[0][0] = cutterXAABB(0,0); nodes[0][1] = cutterXAABB(1,0); nodes[0][2] = cutterXAABB(2,0); // node 0   
    nodes[1][0] = cutterXAABB(0,1); nodes[1][1] = cutterXAABB(1,0); nodes[1][2] = cutterXAABB(2,0); // node 1
    nodes[2][0] = cutterXAABB(0,1); nodes[2][1] = cutterXAABB(1,1); nodes[2][2] = cutterXAABB(2,0); // node 2
    nodes[3][0] = cutterXAABB(0,0); nodes[3][1] = cutterXAABB(1,1); nodes[3][2] = cutterXAABB(2,0); // node 3
    nodes[4][0] = cutterXAABB(0,0); nodes[4][1] = cutterXAABB(1,0); nodes[4][2] = cutterXAABB(2,1); // node 4
    nodes[5][0] = cutterXAABB(0,1); nodes[5][1] = cutterXAABB(1,0); nodes[5][2] = cutterXAABB(2,1); // node 5
    nodes[6][0] = cutterXAABB(0,1); nodes[6][1] = cutterXAABB(1,1); nodes[6][2] = cutterXAABB(2,1); // node 6
    nodes[7][0] = cutterXAABB(0,0); nodes[7][1] = cutterXAABB(1,1); nodes[7][2] = cutterXAABB(2,1); // node 7
    
    for (int i = 0; i < 8; i++)
        if(isPositionWithinXAABB(nodes[i], xfemXAABB))
        {
            intersection = true;
            break;
        }
    
    if(!intersection)
    {
        for (int i = 0; i < 12; i++)
        {
            const int index1 = eleNodeNumbering_hex27_lines[i][0];
            const int index2 = eleNodeNumbering_hex27_lines[i][1];
            if(isLineWithinXAABB(nodes[index1], nodes[index2], xfemXAABB))
            {
                intersection = true;
                break;
            }
        }
    }
    
    if(!intersection)
    {
        nodes[0][0] = xfemXAABB(0,0);   nodes[0][1] = xfemXAABB(1,0);   nodes[0][2] = xfemXAABB(2,0);   // node 0   
        nodes[1][0] = xfemXAABB(0,1);   nodes[1][1] = xfemXAABB(1,0);   nodes[1][2] = xfemXAABB(2,0);   // node 1
        nodes[2][0] = xfemXAABB(0,1);   nodes[2][1] = xfemXAABB(1,1);   nodes[2][2] = xfemXAABB(2,0);   // node 2
        nodes[3][0] = xfemXAABB(0,0);   nodes[3][1] = xfemXAABB(1,1);   nodes[3][2] = xfemXAABB(2,0);   // node 3
        nodes[4][0] = xfemXAABB(0,0);   nodes[4][1] = xfemXAABB(1,0);   nodes[4][2] = xfemXAABB(2,1);   // node 4
        nodes[5][0] = xfemXAABB(0,1);   nodes[5][1] = xfemXAABB(1,0);   nodes[5][2] = xfemXAABB(2,1);   // node 5
        nodes[6][0] = xfemXAABB(0,1);   nodes[6][1] = xfemXAABB(1,1);   nodes[6][2] = xfemXAABB(2,1);   // node 6
        nodes[7][0] = xfemXAABB(0,0);   nodes[7][1] = xfemXAABB(1,1);   nodes[7][2] = xfemXAABB(2,1);   // node 7
    
        for (int i = 0; i < 8; i++)
            if(isPositionWithinXAABB(nodes[i], cutterXAABB))
            {
                intersection = true;
                break;
            }
    }   
    
    if(!intersection)
    {
        for (int i = 0; i < 12; i++)
        {
            const int index1 = eleNodeNumbering_hex27_lines[i][0];
            const int index2 = eleNodeNumbering_hex27_lines[i][1];
            if(isLineWithinXAABB(nodes[index1], nodes[index2], cutterXAABB))
            {
                intersection = true;
                break;
            }
        }
    }
    return intersection;
}


#endif  // #ifdef CCADISCRET
