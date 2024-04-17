/*-----------------------------------------------------------*/
/*! \file

\brief contains base class for a generic output filter (ensight and vtk are derived from this class)


\level 2

*/
/*-----------------------------------------------------------*/

#include "baci_post_filter_base.hpp"

#include "baci_io_legacy_table.hpp"
#include "baci_io_legacy_table_iter.hpp"
#include "baci_post_common.hpp"
#include "baci_post_ensight_writer.hpp"
#include "baci_post_vtk_vti_writer.hpp"
#include "baci_post_vtk_vtu_writer.hpp"
#include "baci_post_vtk_vtu_writer_node_based.hpp"
#include "baci_post_writer_base.hpp"

FOUR_C_NAMESPACE_OPEN

PostFilterBase::PostFilterBase(PostField* field, const std::string& name)
{
  if (field->problem()->filter() == "ensight")
    writer_ = Teuchos::rcp(new EnsightWriter(field, name));
  else if (field->problem()->filter() == "vtu")
    writer_ = Teuchos::rcp(new PostVtuWriter(field, name));
  else if (field->problem()->filter() == "vti")
    writer_ = Teuchos::rcp(new PostVtiWriter(field, name));
  else if (field->problem()->filter() == "vtu_node_based")
    writer_ = Teuchos::rcp(new PostVtuWriterNode(field, name));
  else
    dserror("Unsupported filter");
}



void PostFilterBase::WriteFiles()
{
  dsassert(writer_ != Teuchos::null, "No writer has been set! Fatal error");
  writer_->WriteFiles(*this);
}



void PostFilterBase::WriteAnyResults(PostField* field, const char* type, const ResultType restype)
{
  PostResult result = PostResult(field);
  result.next_result();

  MAP_ITERATOR iter;
  init_map_iterator(&iter, result.group());

  while (next_map_node(&iter))
  {
    // We do not support multiple definitions of the same name here. We just
    // use the map node to obtain the key string. Afterward we can use normal
    // map functions to find out if this key names an element vector group.
    MAP_NODE* node = iterator_get_node(&iter);
    char* key = node->key;
    if (map_has_map(result.group(), key))
    {
      MAP* entry = map_read_map(result.group(), key);
      if (map_has_string(entry, "type", type))
      {
        int dim;
        // This is bad. We should have a generic way to find how many dofs
        // there are. Until then this is remains a special purpose routine
        // that cannot serve everybody.
        if (restype == elementbased)
          // for elements we have the number of columns
          dim = map_read_int(entry, "columns");
        else if (restype == nodebased)
          // for node the number of columns might be a same bet as well
          dim = map_read_int(entry, "columns");
        else
          // Normal dof vectors have ndim dofs per node. (But then there are
          // velocity / pressure vectors and such...)
          dim = field->problem()->num_dim();
        writer_->WriteResult(key, key, restype, dim);
      }
    }
  }
}

FOUR_C_NAMESPACE_CLOSE
