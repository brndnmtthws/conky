/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef theme_presets_manager_H
#define theme_presets_manager_H

#include <git2.h>
#include <map>
#include <string>
#include <vector>
#include <libxml/parser.h>

class abstract_themes_source {
 public:
  abstract_themes_source();
  virtual bool load_themes_db() = 0;  // or reload
  virtual std::map<std::string, std::string> get_themes_db() = 0;
  virtual std::vector<std::string> load_possible_alias_of_themes() = 0;
  virtual std::string get_theme_path_for_alias(std::string alias) = 0;

 protected:
  bool is_dir(std::string dir);
  bool mkdir(std::string path);
};
class base_local_fs_folder_source : public abstract_themes_source {
 public:
  base_local_fs_folder_source(std::string path_to_folder);
  virtual bool load_themes_db();
  virtual std::map<std::string, std::string> get_themes_db();
  virtual std::vector<std::string> load_possible_alias_of_themes();
  virtual std::string get_theme_path_for_alias(std::string alias);

 protected:
  const std::string path_to_folder;
  std::string parse_theme_alias_from_filename(std::string filename);
  std::map<std::string, std::string> themes_db;
};
class repo_local_fs_source : public base_local_fs_folder_source {
 public:
  repo_local_fs_source(std::string path_to_repo);
  virtual bool load_themes_db();
  // virtual std::map<std::string, std::string> get_themes_db();
  // virtual std::vector<std::string> load_possible_alias_of_themes();
  // virtual std::string get_theme_path_for_alias(std::string alias);
 protected:
  std::string check_is_dir_have_config(std::string path);
};
class system_git_repo_source : public repo_local_fs_source {
 public:
  system_git_repo_source(
      std::string pathToNewGitRepo,
      std::string cloneUrlToGitRepo);  // if repo exists url dont used simple
                                       // pull if not git clone
  system_git_repo_source(std::string path_to_git_already_cloned_repo);
  ~system_git_repo_source();
  virtual bool load_themes_db();  // if loded one or more theme returns true
 protected:
  bool pull_changes(git_repository * target, bool use_base_branch = true);
  bool pull_changes();
  bool open_repo(std::string path_to_repo);
  bool is_getted_path_git_repo(std::string path_to_repo);
  git_repository* repo = NULL;
  bool fetch_repo(git_repository * target);
  bool fetch_repo();
  std::string get_head_branch_name_for_repo(git_repository * target);
  bool is_submodule_of_repo(std::string path, git_submodule **result_submodule = nullptr);
  // /std::string _path_to_repo = "";
  std::string repo_url = "";
  //   virtual std::map<std::string, std::string> get_themes_db();
  //   virtual std::vector<std::string> load_possible_alias_of_themes();
  //   virtual std::string get_theme_path_for_alias(std::string alias);
  bool is_getted_initilaized_repo_submodule(git_submodule *submodule, git_repository **repo_result = nullptr);
};


class advanced_git_repo_source: public system_git_repo_source{
 public:
  advanced_git_repo_source(std::string repo_path, std::string repo_url);
  ~advanced_git_repo_source();
  virtual bool load_themes_db();
  virtual std::string get_theme_path_for_alias(std::string alias);
 protected:
  bool set_repo_info_from_info_node(xmlNode * node);
  std::map<std::string, std::string> parse_themes_db_from_xml();
  bool git_init_with_remote_origin(std::string  path_to_repo, std::string remote_origin_url);
  bool git_pull_specific_path(std::string path, bool use_base_branch = true);


  std::string repo_name = "udefinded";
  std::string repo_description = "udefinded";
  xmlDoc * xml_doc = nullptr;
};

class theme_presets_manager {
 public:
  theme_presets_manager(abstract_themes_source*  source);
  std::vector<std::string> get_possible_preload_theme_alias();
  std::string get_theme_path(std::string id);

 private:
  abstract_themes_source*  themes_source = nullptr;
};

#endif  // theme_presets_manager_H
