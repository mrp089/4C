/*!----------------------------------------------------------------------
\file
\brief contains the routine 
 - s9intg: which gets the natural coordinates of the integration points 
           and their weights

*----------------------------------------------------------------------*/
#ifdef D_SHELL9
#include "../headers/standardtypes.h"
#include "shell9.h"

/*! 
\addtogroup SHELL9 
*//*! @{ (documentation module open)*/

/*!----------------------------------------------------------------------
\brief integration points and weights                                      

<pre>                     m.gee 6/01              modified by    sh 10/02
This routine gets the natural coordinates of the integration points and
their weights for a numerical integration
</pre>
\param  const ELEMENT *ele    (i) element array of actual element
\param  S9_DATA       *data   (o) coordinates and weights at GP
\param  INT            option (i) ?

\warning There is nothing special to this routine
\return void                                               
\sa calling: ---; called by: s9eleload()     [s9_load1.c]
                             s9static_keug() [s9_static_keug.c]
                             s9_stress()     [s9_stress.c]

*----------------------------------------------------------------------*/
void s9intg(const ELEMENT   *ele,
            S9_DATA         *data,
            INT              option)
{
DOUBLE b,wgt,wgt0;
#ifdef DEBUG 
dstrc_enter("s9intg");
#endif
/*----------------------------------------------------------------------*/
if (option==0)
{
       switch(ele->e.s9->nGP[2])/*---------------- thickness direction t */
       {
          case 2:
             b   = 1.0/3.0;
             b   = sqrt(b);
             wgt = 1.0;
             data->xgpt[0] = -b;
             data->xgpt[1] =  b;
             data->xgpt[2] =  0.0;
             data->wgtt[0] =  wgt;
             data->wgtt[1] =  wgt;
             data->wgtt[2] =  0.0;
          break;
          default:
              dserror("unknown number of GP points");
          break;
       }
   if (ele->distyp == quad4 || /*---------------- quadrilateral elements */
       ele->distyp == quad8 ||
       ele->distyp == quad9 ) 
   {
       switch(ele->e.s9->nGP[0])/* direction r */
       {
       case 1:
          data->xgpr[0] = 0.0;
          data->xgpr[1] = 0.0;
          data->xgpr[2] = 0.0;
          data->wgtr[0] = 2.0;
          data->wgtr[1] = 0.0;
          data->wgtr[2] = 0.0;
       break;
       case 2:
          b   = 1.0/3.0;
          b   = sqrt(b);
          wgt = 1.0;
          data->xgpr[0] = -b;
          data->xgpr[1] =  b;
          data->xgpr[2] =  0.0;
          data->wgtr[0] =  wgt;
          data->wgtr[1] =  wgt;
          data->wgtr[2] =  0.0;
       break;
       case 3:
          b    = 3.0/5.0;
          b    = sqrt(b);
          wgt  = 5.0/9.0;
          wgt0 = 8.0/9.0;
          data->xgpr[0] = -b;
          data->xgpr[1] =  0.0;
          data->xgpr[2] =  b;
          data->wgtr[0] =  wgt;
          data->wgtr[1] =  wgt0;
          data->wgtr[2] =  wgt;
       break;
       default:
          dserror("unknown number of GP points");
       break;
       }
       switch(ele->e.s9->nGP[1])/* direction s */
       {
       case 1:
          data->xgps[0] = 0.0;
          data->xgps[1] = 0.0;
          data->xgps[2] = 0.0;
          data->wgts[0] = 2.0;
          data->wgts[1] = 0.0;
          data->wgts[2] = 0.0;
       break;
       case 2:
          b   = 1.0/3.0;
          b   = sqrt(b);
          wgt = 1.0;
          data->xgps[0] = -b;
          data->xgps[1] =  b;
          data->xgps[2] =  0.0;
          data->wgts[0] =  wgt;
          data->wgts[1] =  wgt;
          data->wgts[2] =  0.0;
       break;
       case 3:
          b    = 3.0/5.0;
          b    = sqrt(b);
          wgt  = 5.0/9.0;
          wgt0 = 8.0/9.0;
          data->xgps[0] = -b;
          data->xgps[1] =  0.0;
          data->xgps[2] =  b;
          data->wgts[0] =  wgt;
          data->wgts[1] =  wgt0;
          data->wgts[2] =  wgt;
       break;
       default:
          dserror("unknown number of GP points");
       break;
       }
    }
    else if (ele->distyp == tri3 || /*-------------- triangular elements */
             ele->distyp == tri6 )
    {
       switch(ele->e.s9->nGP_tri)
       {
       case 1:
          b   = 1.0/3,0;
          wgt = 1.0/2.0;
          data->xgpr[0] =  b;
          data->xgpr[1] =  0.0;
          data->xgpr[2] =  0.0;
          data->xgps[0] =  b;
          data->xgps[1] =  0.0;
          data->xgps[2] =  0.0;
          data->wgtr[0] =  wgt;
          data->wgtr[1] =  0.0;
          data->wgtr[2] =  0.0;
          data->wgts[0] =  wgt;
          data->wgts[1] =  0.0;
          data->wgts[2] =  0.0;
       break;
       case 3:
          b   = 1.0/2.0;
          wgt = 1.0/6.0;
          data->xgpr[0] =  b;
          data->xgpr[1] =  b;
          data->xgpr[2] =  0.0;
          data->xgps[0] =  0.0;
          data->xgps[1] =  b;
          data->xgps[2] =  b;
          data->wgtr[0] =  wgt;
          data->wgtr[1] =  wgt;
          data->wgtr[2] =  wgt;
          data->wgts[0] =  wgt;
          data->wgts[1] =  wgt;
          data->wgts[2] =  wgt;
       break;
       default:
             dserror("unknown number of GP points");
       break;
       }
    }
}
/*----------------------------------------------------------------------*/
#ifdef DEBUG 
dstrc_exit();
#endif
return;
} /* end of s9intg */
/*----------------------------------------------------------------------*/
#endif /*D_SHELL9*/
/*! @} (documentation module close)*/




