#include "variables.hh"

#include <cassert>
#include <cstring>

#include "logging.h"
#include "update-cb.hh"

namespace conky::text_object {

static variable_map &mutable_map() {
  static variable_map instance;
  return instance;
}

const variable_map &variables() { return mutable_map(); }

const variable_definition *find_variable(std::string_view name) {
  const auto &m = variables();
  auto it = m.find(name);
  return it != m.end() ? &it->second : nullptr;
}

int register_variable_impl(std::initializer_list<variable_definition> entries) {
  auto &m = mutable_map();
  for (const auto &e : entries) {
    auto [it, inserted] = m.emplace(e.name, e);
#ifndef NDEBUG
    if (!inserted) {
      NORM_ERR("text object '%s' registered more than once", e.name);
    }
#endif
  }
  return 0;
}

::text_object *construct_text_object(const char *s, const char *arg, long line,
                                   void **ifblock_opaque,
                                   void *free_at_crash) {
  const auto *entry = find_variable(s);
  if (entry == nullptr) { return nullptr; }

  auto *obj =
      static_cast<::text_object *>(malloc(sizeof(struct text_object)));
  memset(obj, 0, sizeof(struct text_object));
  obj->line = line;

  if (entry->update_cb != nullptr) {
    obj->cb_handle = new legacy_cb_handle(
        conky::register_cb<legacy_cb>(1, entry->update_cb));
  }

  if ((entry->flags & obj_flags::arg) && !arg) {
    free(obj);
    NORM_ERR("'%s' requires an argument", s);
    return nullptr;
  }
  if (entry->flags & obj_flags::cond) {
    obj_be_ifblock_if(ifblock_opaque, obj);
  }

  create_status status = create_status::success;
  construct_context ctx{arg, ifblock_opaque, free_at_crash, &status};
  entry->construct(obj, ctx);

  if (status != create_status::success) {
    free(obj);
    return nullptr;
  }

  return obj;
}

}  // namespace conky::text_object
