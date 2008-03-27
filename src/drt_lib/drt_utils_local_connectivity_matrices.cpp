 /*!
 \file drt_utils_local_connectivity_matrices.cpp

 \brief Provide a node numbering scheme together with a set of shape functions

 Provided are 1D, 2D and 3D shape functions

 The surface mapping gives the node numbers such that the 2D shape functions can be used
 Nodal mappings describe the relation between volume, surface and line node numbering.
 They should be used as the only reference for such relationships.
 The corresponding graphics and a detailed description can be found in the Baci guide in the Convention chapter.
 The numbering of lower order elements is included in the higher order element, such that
 e.g. the hex8 volume element uses only the first 8 nodes of the hex27 mapping

 !!!!
 The corresponding graphics and a detailed description can be found
 in the Baci guide in the Convention chapter.
 !!!!

 \author Axel Gerstenberger
 gerstenberger@lnm.mw.tum.de
 http://www.lnm.mw.tum.de
 089 - 289-15236
 */
#ifdef CCADISCRET

#include "drt_element.H"
#include "drt_discret.H"
#include "drt_utils_local_connectivity_matrices.H"
#include "drt_utils_integration.H"
#include "drt_utils_fem_shapefunctions.H"
#include "drt_dserror.H"

/*----------------------------------------------------------------------*
 |  returns the number of nodes                              a.ger 11/07|
 |  for each discretization type                                        |
 *----------------------------------------------------------------------*/    
int DRT::UTILS::getNumberOfElementNodes( 
    const DRT::Element::DiscretizationType&     distype)
{
    
    int numnodes = 0;
    
    switch(distype)
    {
    case DRT::Element::dis_none:     return 0;    break;
    case DRT::Element::point1:       return 1;    break;
    case DRT::Element::line2:        return 2;    break;
    case DRT::Element::line3:        return 3;    break;
    case DRT::Element::tri3:         return 3;    break;
    case DRT::Element::tri6:         return 6;    break;
    case DRT::Element::quad4:        return 4;    break;
    case DRT::Element::quad8:        return 8;    break;
    case DRT::Element::quad9:        return 9;    break;
    case DRT::Element::hex8:         return 8;    break;
    case DRT::Element::hex20:        return 20;   break;
    case DRT::Element::hex27:        return 27;   break;
    case DRT::Element::tet4:         return 4;    break;
    case DRT::Element::tet10:        return 10;   break;
    default:
        dserror("discretization type not yet implemented");  
    }
    
    return numnodes;     
}   


/*----------------------------------------------------------------------*
 |  returns the number of corner nodes                       u.may 08/07|
 |  for each discretization type                                        |
 *----------------------------------------------------------------------*/    
int DRT::UTILS::getNumberOfElementCornerNodes( 
    const DRT::Element::DiscretizationType&     distype)
{
    
    int numCornerNodes = 0;
    
    switch(distype)
    {
        case DRT::Element::hex8:
            numCornerNodes = 8;
            break;
        case DRT::Element::hex20:
            numCornerNodes = 8;
            break;
        case DRT::Element::hex27:
            numCornerNodes = 8;
            break;
        case DRT::Element::tet4:
            numCornerNodes = 4;
            break;
        case DRT::Element::tet10:
            numCornerNodes = 4;
            break;   
        default:
            dserror("discretization type not yet implemented");     
    }
    
    return numCornerNodes;     
}   
    


/*----------------------------------------------------------------------*
 |  returns the number of lines                              a.ger 08/07|
 |  for each discretization type                                        |
 *----------------------------------------------------------------------*/    
int DRT::UTILS::getNumberOfElementLines( 
    const DRT::Element::DiscretizationType&     distype)
{
    int numLines = 0;
    switch(distype)
    {
        case DRT::Element::hex8: case DRT::Element::hex20: case DRT::Element::hex27:
            numLines = 12;
            break;
        case DRT::Element::wedge6: case DRT::Element::wedge15:
            numLines = 9;
            break;
        case DRT::Element::tet4: case DRT::Element::tet10:
            numLines = 6;
            break;
        case DRT::Element::quad4: case DRT::Element::quad8: case DRT::Element::quad9:
            numLines = 4;
            break;
        case DRT::Element::tri3: case DRT::Element::tri6:
            numLines = 3;
            break;
        default:
            dserror("discretization type not yet implemented");     
    }
    return numLines;
}  


/*----------------------------------------------------------------------*
 |  returns the number of lines                              a.ger 08/07|
 |  for each discretization type                                        |
 *----------------------------------------------------------------------*/    
int DRT::UTILS::getNumberOfElementSurfaces( 
    const DRT::Element::DiscretizationType&     distype)
{
    int numSurf = 0;
    switch(distype)
    {
        case DRT::Element::hex8: case DRT::Element::hex20: case DRT::Element::hex27:
            numSurf = 6;
            break;
        case DRT::Element::wedge6: case DRT::Element::wedge15:
            numSurf = 5;
            break;
        case DRT::Element::tet4: case DRT::Element::tet10:
            numSurf = 4;
            break;
        default:
            dserror("discretization type not yet implemented");     
    }
    return numSurf;
}  

/*----------------------------------------------------------------------*
 |  Fills a vector< vector<int> > with all nodes for         u.may 08/07|
 |  every surface for each discretization type                          |
 *----------------------------------------------------------------------*/    
vector< vector<int> > DRT::UTILS::getEleNodeNumberingSurfaces(   
    const DRT::Element::DiscretizationType&     distype)
{   
    int nSurf;
    int nNode; 
    vector< vector<int> >   map;
   
    
    switch(distype)
    {
        case DRT::Element::hex8:
        {
            nSurf = 6;
            nNode = 4;
            
            vector<int> submap(nNode, 0);
            for(int i = 0; i < nSurf; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_surfaces[i][j];
            }
            break;
        }
        case DRT::Element::hex20:
        {
            nSurf = 6;
            nNode = 8;
           
            vector<int> submap(nNode, 0);          
            for(int i = 0; i < nSurf; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_surfaces[i][j];
            }
            break;
        }
        case DRT::Element::hex27:
        {
            nSurf = 6;
            nNode = 9;
                     
            vector<int> submap(nNode, 0);
            for(int i = 0; i < nSurf; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_surfaces[i][j];
            }
            break;
        }
        case DRT::Element::tet4:
        {
            nSurf = 4;
            nNode = 3;
          
            vector<int> submap(nNode, 0);           
            for(int i = 0; i < nSurf; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tet10_surfaces[i][j];       
            }
            break;
        }
        case DRT::Element::tet10:
        {
            nSurf = 4;
            nNode = 6;
     
            vector<int> submap(nNode, 0);
            for(int i = 0; i < nSurf; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tet10_surfaces[i][j];
            }
            break;
        }
        case DRT::Element::wedge6:
        {
            // quad surfaces first
            int nqSurf = 3;
            nNode = 4;
            
            vector<int> submapq(nNode, 0);
            for(int i = 0; i < nqSurf; i++)
            {
                map.push_back(submapq);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_wedge15_quadsurfaces[i][j];
            }
            int ntSurf = 2;
            nNode = 3;
            
            vector<int> submapt(nNode, 0);
            for(int i = 0; i < ntSurf; i++)
            {
                map.push_back(submapt);
                for(int j = 0; j < nNode; j++)
                    map[i+nqSurf][j] = eleNodeNumbering_wedge15_trisurfaces[i][j];
            }
            break;
        }
        case DRT::Element::pyramid5:
        {
          // quad surfaces first
          nSurf = 1;
          nNode = 4;
          
          vector<int> submapq(nNode, 0);
          for(int i = 0; i < nSurf; i++)
          {
              map.push_back(submapq);
              for(int j = 0; j < nNode; j++)
                  map[i][j] = eleNodeNumbering_pyramid5_quadsurfaces[i][j];
          }
          nSurf = 4;
          nNode = 3;
          
          vector<int> submapt(nNode, 0);
          for(int i = 0; i < nSurf; i++)
          {
              map.push_back(submapt);
              for(int j = 0; j < nNode; j++)
                  map[i+1][j] = eleNodeNumbering_pyramid5_trisurfaces[i][j];
          }
          break;
        }
        default: 
            dserror("discretizationtype is not yet implemented"); 
    }
    
    return map;
}
    



   
/*----------------------------------------------------------------------*
 |  Fills a vector< vector<int> > with all nodes for         u.may 08/07|
 |  every line for each discretization type                             |
 *----------------------------------------------------------------------*/      
vector< vector<int> > DRT::UTILS::getEleNodeNumberingLines(  
    const DRT::Element::DiscretizationType&     distype)
{   
    int nLine;
    int nNode;
    
    vector< vector<int> >  map;
        
    switch(distype)
    {
        case DRT::Element::hex8:
        {
            nLine = 12;
            nNode = 2;          
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_lines[i][j];
            }
            break;
        }
        case DRT::Element::hex20:
        {
            nLine = 12;
            nNode = 3;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {   
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_lines[i][j];
            }
            break;
        }
        case DRT::Element::hex27:
        {
            nLine = 12;
            nNode = 3;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_hex27_lines[i][j];
            }
            break;
        }
        case DRT::Element::tet4:
        {
            nLine = 6;
            nNode = 2;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tet10_lines[i][j];
            }
            break;
        }
        case DRT::Element::tet10:
        {
            nLine = 6;
            nNode = 3;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tet10_lines[i][j];
            }
            break;
        }
        case DRT::Element::quad9:
        {
            nLine = 4;
            nNode = 3;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_quad9_lines[i][j];
            }
            break;
        } 
        case DRT::Element::quad4:
        {
            nLine = 4;
            nNode = 2;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_quad9_lines[i][j];
            }
            break;
        }
        case DRT::Element::tri6:
        {
            nLine = 3;
            nNode = 3;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tri6_lines[i][j];
            }
            break;
        }
        case DRT::Element::tri3:
        {
            nLine = 3;
            nNode = 2;
            vector<int> submap(nNode, 0);
            
            for(int i = 0; i < nLine; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nNode; j++)
                    map[i][j] = eleNodeNumbering_tri6_lines[i][j];
            }
            break;
        }
        default: 
            dserror("discretizationtype is not yet implemented"); 
    }
    
    return map;
}






/*----------------------------------------------------------------------*
 |  Fills a vector< vector<int> > with all surfaces for      u.may 08/07|
 |  every line for each discretization type                             |
 *----------------------------------------------------------------------*/   
vector< vector<int> > DRT::UTILS::getEleNodeNumbering_lines_surfaces(    
    const DRT::Element::DiscretizationType&     distype)
{   
    int nLine;
    int nSurf;
       
    vector< vector<int> > map;
        
    if(distype == DRT::Element::hex8 ||  distype == DRT::Element::hex20 || distype == DRT::Element::hex27)
    {
        nLine = 12;
        nSurf = 2;
        vector<int> submap(nSurf, 0);
        for(int i = 0; i < nLine; i++)
        { 
            map.push_back(submap);
            for(int j = 0; j < nSurf; j++)
                map[i][j] = eleNodeNumbering_hex27_lines_surfaces[i][j];
        }
    }
    else if(distype == DRT::Element::tet4 ||  distype == DRT::Element::tet10)
    {
        nLine = 6;
        nSurf = 2;
        vector<int> submap(nSurf, 0);
        for(int i = 0; i < nLine; i++)
        { 
            map.push_back(submap);
            for(int j = 0; j < nSurf; j++)
                map[i][j] = eleNodeNumbering_tet10_lines_surfaces[i][j];
        }
    }
    else
        dserror("discretizationtype not yet implemented");
        
        
    return map;
    
}
                                  
             
                                               
/*----------------------------------------------------------------------*
 |  Fills a vector< vector<int> > with all surfaces for      u.may 08/07|
 |  every node for each discretization type                             |
 *----------------------------------------------------------------------*/   
vector< vector<int> > DRT::UTILS::getEleNodeNumbering_nodes_surfaces(    
    const DRT::Element::DiscretizationType      distype)
{
    int nNode;
    int nSurf;
       
    vector< vector<int> >   map;
        
    if(distype == DRT::Element::hex8 ||  distype == DRT::Element::hex20 || distype == DRT::Element::hex27)
    {
        nNode = 8;
        nSurf = 3;
        vector<int> submap(nSurf, 0);
        for(int i = 0; i < nNode; i++)
        {
            map.push_back(submap);
            for(int j = 0; j < nSurf; j++)
                map[i][j] = eleNodeNumbering_hex27_nodes_surfaces[i][j];
        }
    }
    else if(distype == DRT::Element::tet4 ||  distype == DRT::Element::tet10)
    {
        nNode = 4;
        nSurf = 3;
        vector<int> submap(nSurf, 0);
        for(int i = 0; i < nNode; i++)
        {
            map.push_back(submap);
            for(int j = 0; j < nSurf; j++)
                map[i][j] = eleNodeNumbering_tet10_nodes_surfaces[i][j];
        }
    }
    else
        dserror("discretizationtype not yet implemented");
        
    return map; 
   
}



/*----------------------------------------------------------------------*
 |  Fills a vector< vector<int> > with all nodes for         u.may 08/07|
 |  every surface for each discretization type                          |
 *----------------------------------------------------------------------*/   
vector< vector<double> > DRT::UTILS::getEleNodeNumbering_nodes_reference(   
    const DRT::Element::DiscretizationType      distype)
{

    int nNode;
    int nCoord;
    
    vector< vector<double> >   map;
        
    switch(distype)
    {
        case DRT::Element::hex8:
        {
            nNode = 8;
            nCoord = 3;
            vector<double> submap(nCoord, 0);
            for(int i = 0; i < nNode; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nCoord; j++)
                    map[i][j] = eleNodeNumbering_hex27_nodes_reference[i][j];
            }
            break;
        }
        case DRT::Element::hex20:
        {
            nNode = 20;
            nCoord = 3;
            vector<double> submap(nCoord, 0);
            for(int i = 0; i < nNode; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nCoord; j++)
                    map[i][j] = eleNodeNumbering_hex27_nodes_reference[i][j];
            }
            break;
        }
        case DRT::Element::hex27:
        {
            nNode = 27;
            nCoord = 3;
            vector<double> submap(nCoord, 0);
            for(int i = 0; i < nNode; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nCoord; j++)
                    map[i][j] = eleNodeNumbering_hex27_nodes_reference[i][j];
            }
            break;
        }
        case DRT::Element::tet4:
        {
            nNode = 4;
            nCoord = 3;
            vector<double> submap(nCoord, 0);
            for(int i = 0; i < nNode; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nCoord; j++)
                    map[i][j] = eleNodeNumbering_tet10_nodes_reference[i][j];
            }
            break;
        }
        case DRT::Element::tet10:
        {
            nNode = 10;
            nCoord = 3;
            vector<double> submap(nCoord, 0);
            for(int i = 0; i < nNode; i++)
            {
                map.push_back(submap);
                for(int j = 0; j < nCoord; j++)
                    map[i][j] = eleNodeNumbering_tet10_nodes_reference[i][j];
            }
            break;
        }
        default:
            dserror("discretizationtype not yet implemented");
    }
    
    return map;
}


/*----------------------------------------------------------------------*
 |  Fills an array with surface ID s a point is lying on     u.may 08/07|
 |  for each discretization type                                        |
 *----------------------------------------------------------------------*/   
int DRT::UTILS::getSurfaces(    
    const blitz::Array<double,1>&               rst,
    int*                                        surfaces,
    const DRT::Element::DiscretizationType      distype)
{

    int countSurf = 0;
    double TOL = 1e-7;
    
    if(distype == DRT::Element::hex8 ||  distype == DRT::Element::hex20 || distype == DRT::Element::hex27)
    {
        if(fabs(rst(0)-1.0) < TOL)      surfaces[countSurf++] = 2;        
        if(fabs(rst(0)+1.0) < TOL)      surfaces[countSurf++] = 4;        
        if(fabs(rst(1)-1.0) < TOL)      surfaces[countSurf++] = 3;        
        if(fabs(rst(1)+1.0) < TOL)      surfaces[countSurf++] = 1;        
        if(fabs(rst(2)-1.0) < TOL)      surfaces[countSurf++] = 5;        
        if(fabs(rst(2)+1.0) < TOL)      surfaces[countSurf++] = 0;     
    }
    else if(distype == DRT::Element::tet4 ||  distype == DRT::Element::tet10 )
    {
        const double tetcoord = rst(0)+rst(1)+rst(2);
        if(fabs(rst(1))         < TOL)  surfaces[countSurf++] = 0;        
        if(fabs(tetcoord-1.0)   < TOL)  surfaces[countSurf++] = 1;       
        if(fabs(rst(0))         < TOL)  surfaces[countSurf++] = 2;        
        if(fabs(rst(2))         < TOL)  surfaces[countSurf++] = 3;          
    }
    else
        dserror("discretization type not yet implemented");
        
    return countSurf;   
}


/*----------------------------------------------------------------------*
 |  Fills an array with coordinates in the reference         u.may 08/07|
 |  system of the cutter element                                        |
 |  according to the node ID for each discretization type               |
 *----------------------------------------------------------------------*/   
void DRT::UTILS::getNodeCoordinates(    const int                                   nodeId,
                                        double*                                     coord,
                                        const DRT::Element::DiscretizationType      distype)
{

    if(distype == DRT::Element::quad4 ||  distype == DRT::Element::quad8 || distype == DRT::Element::quad9)
    {       
        switch(nodeId)
        {
            case 0:
            {
                coord[0] = -1.0; 
                coord[1] = -1.0;
                break;
            }
            case 1:
            {
                coord[0] =  1.0; 
                coord[1] = -1.0;
                break;
            }
            case 2:
            {
                coord[0] =  1.0; 
                coord[1] =  1.0;
                break;
            }
            case 3:
            {
                coord[0] = -1.0; 
                coord[1] =  1.0;
                break;
            }
            default:
                dserror("node number not correct");    
        }
        coord[2] = 0.0; 
    }
    else if(distype == DRT::Element::tri3 ||  distype == DRT::Element::tri6)
    {       
        switch(nodeId)
        {
            case 0:
            {
                coord[0] = 0.0; 
                coord[1] = 0.0;
                break;
            }
            case 1:
            {
                coord[0] =  1.0; 
                coord[1] =  0.0;
                break;
            }
            case 2:
            {
                coord[0] =  0.0; 
                coord[1] =  1.0;
                break;
            }
            default:
                dserror("node number not correct");    
        }
        coord[2] = 0.0; 
    }
    else dserror("discretizationtype is not yet implemented");
}



/*----------------------------------------------------------------------*
 |  Fills an array with coordinates in the reference         u.may 08/07|
 |  system of the cutter element                                        |
 |  according to the line ID for each discretization type               |
 *----------------------------------------------------------------------*/   
void DRT::UTILS::getLineCoordinates(    
    const int                                   lineId,
    const double                                lineCoord,
    double*                                     coord,
    const DRT::Element::DiscretizationType      distype)
{

    if(distype == DRT::Element::quad4 ||  distype == DRT::Element::quad8 || distype == DRT::Element::quad9)
    {
        // change minus sign if you change the line numbering   
        switch(lineId)
        {
            case 0:
            {
                coord[0] = lineCoord; 
                coord[1] = -1.0;
                break;
            }
            case 1:
            {
                coord[0] = 1.0; 
                coord[1] = lineCoord;
                break;
            }
            case 2:
            {
                coord[0] =  -lineCoord;    
                coord[1] =  1.0;
                break;
            }
            case 3:
            {
                coord[0] = -1.0; 
                coord[1] = -lineCoord;
                break;
            }
            default:
                dserror("node number not correct");   
                           
        }  
        coord[2] =  0.0;          
    }
    else if(distype == DRT::Element::tri3 ||  distype == DRT::Element::tri6)
    {
        // change minus sign if you change the line numbering   
        switch(lineId)
        {
            case 0:
            {
                coord[0] = (lineCoord+1)*0.5; 
                coord[1] = 0.0;
                break;
            }
            case 1:
            {
                coord[0] = 1.0; 
                coord[1] = (lineCoord+1)*0.5;
                break;
            }
            case 2:
            {
                coord[0] =  1.0 - (lineCoord+1)*0.5;    
                coord[1] =  (lineCoord+1)*0.5;
                break;
            }
            default:
                dserror("node number not correct");   
                           
        }  
        coord[2] =  0.0;          
    }
    else
        dserror("discretization type not yet implemented");
}



/*----------------------------------------------------------------------*
 |  returns the index of a higher order                      u.may 09/07|
 |  element node index lying between two specified corner               |
 |  node indices for each discretizationtype                            |
 *----------------------------------------------------------------------*/  
int DRT::UTILS::getHigherOrderIndex(
    const int                                   index1, 
    const int                                   index2,
    const DRT::Element::DiscretizationType      distype )
{

    int higherOrderIndex = 0;
    
    switch(distype)
    {
        case DRT::Element::tet10:
        {
            if     ( (index1 == 0 && index2 == 1) || (index1 == 1 && index2 == 0) )      higherOrderIndex = 4;
            else if( (index1 == 1 && index2 == 2) || (index1 == 2 && index2 == 1) )      higherOrderIndex = 5;
            else if( (index1 == 2 && index2 == 0) || (index1 == 0 && index2 == 2) )      higherOrderIndex = 6;
            else if( (index1 == 0 && index2 == 3) || (index1 == 3 && index2 == 0) )      higherOrderIndex = 7;
            else if( (index1 == 1 && index2 == 3) || (index1 == 3 && index2 == 1) )      higherOrderIndex = 8;
            else if( (index1 == 2 && index2 == 3) || (index1 == 3 && index2 == 2) )      higherOrderIndex = 9;
            else dserror("no valid tet10 edge found");
            break;
        }
        case DRT::Element::quad9:
        {
            if     ( (index1 == 0 && index2 == 1) || (index1 == 1 && index2 == 0) )      higherOrderIndex = 4;
            else if( (index1 == 1 && index2 == 2) || (index1 == 2 && index2 == 1) )      higherOrderIndex = 5;
            else if( (index1 == 2 && index2 == 3) || (index1 == 3 && index2 == 2) )      higherOrderIndex = 6;
            else if( (index1 == 3 && index2 == 0) || (index1 == 0 && index2 == 3) )      higherOrderIndex = 7;
            else dserror("no valid quad9 edge found");
            break;
        }
        case DRT::Element::tri6:
        {
            if     ( (index1 == 0 && index2 == 1) || (index1 == 1 && index2 == 0) )      higherOrderIndex = 3;
            else if( (index1 == 1 && index2 == 2) || (index1 == 2 && index2 == 1) )      higherOrderIndex = 4;
            else if( (index1 == 2 && index2 == 0) || (index1 == 0 && index2 == 2) )      higherOrderIndex = 5;
            else dserror("no valid tri6 edge found");
            break;
        }
        default:
            dserror("discretizationtype not yet implemented");  
    }          
    return higherOrderIndex;
}



/*----------------------------------------------------------------------*
 |  returns the dimension of the element parameter space     u.may 10/07|
 *----------------------------------------------------------------------*/  
int DRT::UTILS::getDimension(
    const DRT::Element*   element)
{
    return getDimension(element->Shape());
}

/*----------------------------------------------------------------------*
 |  returns the dimension of the element-shape                 bos 01/08|
 *----------------------------------------------------------------------*/  
int DRT::UTILS::getDimension(const DRT::Element::DiscretizationType distype)
{
    int dim = 0;
    
    switch(distype)
    {
        case DRT::Element::line2 :  case DRT::Element::line3 :
        {
            dim = 1;      
            break;
        }
        case DRT::Element::quad4 : case DRT::Element::quad8 : case DRT::Element::quad9 :
        case DRT::Element::tri3 : case DRT::Element::tri6 :
        {
            dim = 2;   
            break;
        }
        case DRT::Element::hex8 : case DRT::Element::hex20 : case DRT::Element::hex27 :
        case DRT::Element::tet4 : case DRT::Element::tet10 :
        {
            dim = 3;      
            break;
        }   
        default:
            dserror("discretization type is not yet implemented");
    }
    return dim;
}


//
// check for element rewinding based on Jacobian determinant
//
bool DRT::UTILS::checkRewinding3D(const DRT::Element* ele)
{
  const DRT::Element::DiscretizationType distype = ele->Shape();
  const int iel = ele->NumNode();
  // use one point gauss rule to calculate tau at element center
  DRT::UTILS::GaussRule3D integrationrule_1point = DRT::UTILS::intrule3D_undefined;
  switch(distype)
  {
  case DRT::Element::hex8: case DRT::Element::hex20: case DRT::Element::hex27:
      integrationrule_1point = intrule_hex_1point;
      break;
  case DRT::Element::tet4: case DRT::Element::tet10:
      integrationrule_1point = intrule_tet_1point;
      break;
  case DRT::Element::wedge6: case DRT::Element::wedge15:
      integrationrule_1point = intrule_wedge_1point;
      break;
  case DRT::Element::pyramid5:
      integrationrule_1point = intrule_pyramid_1point;
      break;
  default:
      dserror("invalid discretization type for fluid3");
  }
  const IntegrationPoints3D  intpoints = getIntegrationPoints3D(integrationrule_1point);

  // shape functions derivatives
  const int NSD = 3;
  Epetra_SerialDenseMatrix    deriv(NSD, iel);
  Epetra_SerialDenseMatrix    xyze(NSD,iel);
  DRT::UTILS::shape_function_3D_deriv1(deriv,intpoints.qxg[0][0],intpoints.qxg[0][1],intpoints.qxg[0][2],distype);
  // get node coordinates
  const DRT::Node*const* nodes = ele->Nodes();
  for (int inode=0; inode<iel; inode++)
  {
    const double* x = nodes[inode]->X();
    xyze(0,inode) = x[0];
    xyze(1,inode) = x[1];
    xyze(2,inode) = x[2];
  }

  // get Jacobian matrix and determinant
  // actually compute its transpose....
  /*
    +-            -+ T      +-            -+
    | dx   dx   dx |        | dx   dy   dz |
    | --   --   -- |        | --   --   -- |
    | dr   ds   dt |        | dr   dr   dr |
    |              |        |              |
    | dy   dy   dy |        | dx   dy   dz |
    | --   --   -- |   =    | --   --   -- |
    | dr   ds   dt |        | ds   ds   ds |
    |              |        |              |
    | dz   dz   dz |        | dx   dy   dz |
    | --   --   -- |        | --   --   -- |
    | dr   ds   dt |        | dt   dt   dt |
    +-            -+        +-            -+
  */
  Epetra_SerialDenseMatrix xjm(NSD,NSD);

  xjm.Multiply('N','T',1.0,deriv,xyze,0.0);


  
  const double det = xjm(0,0)*xjm(1,1)*xjm(2,2)+
                     xjm(0,1)*xjm(1,2)*xjm(2,0)+
                     xjm(0,2)*xjm(1,0)*xjm(2,1)-
                     xjm(0,2)*xjm(1,1)*xjm(2,0)-
                     xjm(0,0)*xjm(1,2)*xjm(2,1)-
                     xjm(0,1)*xjm(1,0)*xjm(2,2);
  if (abs(det) < 1E-16) dserror("ZERO JACOBIAN DETERMINANT");
  
  bool rewind = false;
  if (det < 0.0) rewind = true;
  else rewind = false;

  return rewind;
}

void DRT::UTILS::Rewinding3D(DRT::Discretization& dis)
{
  cout << "In the Rewinding3D function" << endl;
  

  bool dofillcompleteagain = false;
  //-------------------- loop all my column elements and check rewinding
  for (int i=0; i<dis.NumMyColElements(); ++i)
  {
    // get the actual element
    
	DRT::Element *ele = dis.lColElement(i);
    const DRT::Element::DiscretizationType distype = ele->Shape();
    bool possiblytorewind = false;
    switch(distype)
    {
    case DRT::Element::hex8: case DRT::Element::hex20: case DRT::Element::hex27:
        possiblytorewind = true;
        break;
    case DRT::Element::tet4: case DRT::Element::tet10:
        possiblytorewind = true;
        break;
    case DRT::Element::wedge6: case DRT::Element::wedge15:
        possiblytorewind = true;
        break;
    case DRT::Element::pyramid5:
        possiblytorewind = true;
        break;
    default:
        dserror("invalid discretization type");
    }
    
    if (possiblytorewind) {
      const bool rewind = checkRewinding3D(ele);

      if (rewind) {
        if (distype==DRT::Element::tet4){
          int iel = ele->NumNode();
          vector<int> new_nodeids(iel);
          const int* old_nodeids;
          old_nodeids = ele->NodeIds();
          // rewinding of nodes to arrive at mathematically positive element
          new_nodeids[0] = old_nodeids[0];
          new_nodeids[1] = old_nodeids[2];
          new_nodeids[2] = old_nodeids[1];
          new_nodeids[3] = old_nodeids[3];
          ele->SetNodeIds(iel, &new_nodeids[0]);
        }
        else if (distype==DRT::Element::tet10){
          int iel = ele->NumNode();
          vector<int> new_nodeids(iel);
          const int* old_nodeids;
          old_nodeids = ele->NodeIds();
          // rewinding of nodes to arrive at mathematically positive element
          new_nodeids[0] = old_nodeids[0];
          new_nodeids[1] = old_nodeids[2];
          new_nodeids[2] = old_nodeids[1];
          new_nodeids[3] = old_nodeids[3];
          new_nodeids[4] = old_nodeids[6];
          new_nodeids[5] = old_nodeids[5];
          new_nodeids[6] = old_nodeids[4];
          new_nodeids[7] = old_nodeids[7];
          new_nodeids[8] = old_nodeids[8];
          new_nodeids[9] = old_nodeids[9];          
          ele->SetNodeIds(iel, &new_nodeids[0]);
        }
        else if (distype==DRT::Element::hex8){
          int iel = ele->NumNode();
          vector<int> new_nodeids(iel);
          const int* old_nodeids;
          old_nodeids = ele->NodeIds();
          // rewinding of nodes to arrive at mathematically positive element
          new_nodeids[0] = old_nodeids[4];
          new_nodeids[1] = old_nodeids[5];
          new_nodeids[2] = old_nodeids[6];
          new_nodeids[3] = old_nodeids[7];
          new_nodeids[4] = old_nodeids[0];
          new_nodeids[5] = old_nodeids[1];
          new_nodeids[6] = old_nodeids[2];
          new_nodeids[7] = old_nodeids[3];
          ele->SetNodeIds(iel, &new_nodeids[0]);
        }
        else if (distype==DRT::Element::wedge6){
          int iel = ele->NumNode();
          vector<int> new_nodeids(iel);
          const int* old_nodeids;
          old_nodeids = ele->NodeIds();
          // rewinding of nodes to arrive at mathematically positive element
          new_nodeids[0] = old_nodeids[3];
          new_nodeids[1] = old_nodeids[4];
          new_nodeids[2] = old_nodeids[5];
          new_nodeids[3] = old_nodeids[0];
          new_nodeids[4] = old_nodeids[1];
          new_nodeids[5] = old_nodeids[2];
          ele->SetNodeIds(iel, &new_nodeids[0]);
        }
        else if (distype == DRT::Element::pyramid5){
          int iel = ele->NumNode();
          vector<int> new_nodeids(iel);
          const int* old_nodeids;
          old_nodeids = ele->NodeIds();
          // rewinding of nodes to arrive at mathematically positive element
          new_nodeids[1] = old_nodeids[3];
          new_nodeids[3] = old_nodeids[1];
          // the other nodes can stay the same
          new_nodeids[0] = old_nodeids[0];
          new_nodeids[2] = old_nodeids[2];
          new_nodeids[4] = old_nodeids[4];
          ele->SetNodeIds(iel, &new_nodeids[0]);
        }
        else dserror("no rewinding scheme for this type of element");
        
      }
      // process of rewinding done
      dofillcompleteagain = true;
    }
  }

  return;
}

/*----------------------------------------------------------------------*
 |  Returns the geometric center of the element in local coordinates    |     
 |                                                           a.ger 12/07|
 *----------------------------------------------------------------------*/
blitz::Array<double,1> DRT::UTILS::getLocalCenterPosition(
        const DRT::Element::DiscretizationType   distype     ///< shape of the element
        )
{
    const int dim = getDimension(distype);
    blitz::Array<double,1> pos(dim);
    switch(distype)
    {
        case DRT::Element::line2 :  case DRT::Element::line3 :
        {
            pos = 0.0;
            break;
        }
        case DRT::Element::quad4 : case DRT::Element::quad8 : case DRT::Element::quad9 :
        {
            pos = 0.0;
            break;
        }
        case DRT::Element::tri3 : case DRT::Element::tri6 :
        {
            pos = 1.0/3.0;
            break;
        }
        case DRT::Element::hex8 : case DRT::Element::hex20 : case DRT::Element::hex27 :
        {
            pos = 0.0;
            break;
        }
        case DRT::Element::tet4 : case DRT::Element::tet10 :
        {
            pos = 1.0/4.0;
            break;
        }   
        default:
            dserror("discretization type is not yet implemented");
    }
    return pos;
}



#endif  // #ifdef CCADISCRET
