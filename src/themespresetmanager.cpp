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
#include "themespresetmanager.h"
#include <git2.h>
#include <git2/common.h>
#include <sys/stat.h>
#include <cstdlib>
#include <filesystem>
#include <iostream>
const char *THEME_END_NAME = ".theme";   // can be changed at another one
const char *THEME_CONFIG_END = ".conf";  // can be changed at another one

std::string getStdoutFromCommand(
    std::string cmd) {  // deprecated, need if use system and need to working
                        // with bash stdoutd

  std::string data;
  FILE *stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  cmd.append(" 2>&1");

  stream = popen(cmd.c_str(), "r");

  if (stream) {
    while (!feof(stream))
      if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
    pclose(stream);
  }
  return data;
}

theme_presets_manager::theme_presets_manager(
    abstract_themes_source *source) {
  themes_source = source;
}


std::vector<std::string> theme_presets_manager::get_possible_preload_theme_alias() {
  return themes_source->load_possible_alias_of_themes();
}

std::string theme_presets_manager::get_theme_path(std::string id) {
  return themes_source->get_theme_path_for_alias(id);
}

abstract_themes_source::abstract_themes_source() {}

bool abstract_themes_source::is_dir(std::string path) {
  struct stat info;
  if (stat(path.c_str(), &info) == 0) {
    if (!(info.st_mode & S_IFDIR)) { return false; }
  } else {
    return false;
  }
  return true;
}

bool abstract_themes_source::mkdir(std::string path) {
  return std::filesystem::create_directory(path);
}

base_local_fs_folder_source::base_local_fs_folder_source(
    std::string path_to_folder)
    : abstract_themes_source(), path_to_folder(path_to_folder) {}

bool base_local_fs_folder_source::load_themes_db() {
  if (!is_dir(path_to_folder)) { return false; }

  for (const auto &entry : std::filesystem::directory_iterator(path_to_folder)) {
    std::string themeAlias = parse_theme_alias_from_filename(entry.path());
    if (themeAlias != "") { themes_db[themeAlias] = entry.path(); }
  }
  return !themes_db.empty();
}

std::map<std::string, std::string>
base_local_fs_folder_source::get_themes_db() {
  return themes_db;
}

std::vector<std::string>
base_local_fs_folder_source::load_possible_alias_of_themes() {
  std::vector<std::string> result;
  for (std::pair<std::string, std::string> theme : themes_db) {
    result.push_back(theme.first);
  }
  return result;
}

std::string base_local_fs_folder_source::get_theme_path_for_alias(
    std::string alias) {
  if (themes_db.empty() | !themes_db.count(alias)) {
    return "";
  } else {
    return themes_db[alias];
  }
}

std::string base_local_fs_folder_source::parse_theme_alias_from_filename(
    std::string filename) {
  if (filename.find(THEME_END_NAME) == filename.npos) {
    std::cout << "base_local_fs_folder_source. cant parse aloas from filename"
              << std::endl;
    return "";
  }
  if (filename.rfind("/") != filename.npos) {
    filename = filename.substr(filename.rfind("/") + 1, filename.size());
    // std::cout << filename << std::endl;
  }
  return filename.substr(0, filename.find(THEME_END_NAME));
}

repo_local_fs_source::repo_local_fs_source(std::string path_to_repo)
    : base_local_fs_folder_source(path_to_repo) {}

bool repo_local_fs_source::load_themes_db() {
  if (!is_dir(path_to_folder)) { return false; }
  for (const auto &entry : std::filesystem::directory_iterator(path_to_folder)) {
    std::string configPath = check_is_dir_have_config(entry.path());
    if (configPath != "") {
      themes_db[parse_theme_alias_from_filename(entry.path())] = configPath;
    }
  }
  if (themes_db.empty()) { return false; }
  return true;
}

std::string repo_local_fs_source::check_is_dir_have_config(std::string path) {
  if (!is_dir(path)) { return ""; }
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    std::string filename = entry.path();
    filename = filename.substr(
        filename.rfind("/") + 1,
        filename.size());  // remove dirs and get simply filename
    if (filename.find(THEME_CONFIG_END) != filename.npos) {
      return entry.path();
    }
  }
  return "";  // cant find config
}
void show_last_git_error(std::string prefix){
  std::cout << prefix << " log: " << git_error_last()->message << std::endl;
}

system_git_repo_source::system_git_repo_source(std::string pathToNewGitRepo,
                                               std::string cloneUrlToGitRepo)
    : repo_local_fs_source(pathToNewGitRepo), repo_url(cloneUrlToGitRepo) {
  git_libgit2_init();
}

system_git_repo_source::system_git_repo_source(
    std::string path_to_git_already_cloned_repo)
    : repo_local_fs_source(path_to_git_already_cloned_repo) {
  git_libgit2_init();
}

system_git_repo_source::~system_git_repo_source() {
  git_libgit2_shutdown();
}

bool system_git_repo_source::load_themes_db() {
  if (!is_dir(path_to_folder)) {
    if (!mkdir(path_to_folder)) {
      std::cout
          << "system_git_repo_source::path dir repo not exists and cant create"
          << std::endl;
      return false;
    }
  }
  if (is_getted_path_git_repo(path_to_folder)) {
    if (open_repo(path_to_folder)) {
      if (pull_changes()) { return repo_local_fs_source::load_themes_db(); }
      return false;
    }
    return false;
  } else {
    if (repo_url != "") {
      std::cout << "cloning" << std::endl;
      if (git_clone(&repo, repo_url.c_str(), path_to_folder.c_str(), NULL) !=
          0) {
        std::cout << "system_git_repo_source::error while git clone(init url): "
                  << git_error_last()->message << std::endl;
        return false;
      }
      return repo_local_fs_source::load_themes_db();
    }
  }
  return false;
}
const char * BASE_REPO_BRANCH = "main";
bool system_git_repo_source::pull_changes(git_repository *target, bool use_base_branch)  // maybe need to change this
{
  if (!fetch_repo()){
    show_last_git_error("Error while fetch");
    return false;
  }
  git_reference *mainBranch;
  std::string branch_name = BASE_REPO_BRANCH;
  if (!use_base_branch){
    branch_name = get_head_branch_name_for_repo(target);
  }

  if (git_reference_lookup(&mainBranch, target, (std::string("refs/remotes/origin/") + branch_name).c_str()) !=
      0) {
    std::cout << "system_git_repo_source::error while git lookup (main): "
              << git_error_last()->message << std::endl;
    return false;
  }
  git_annotated_commit *lastMainBranchCommit;
  if (git_annotated_commit_from_ref(&lastMainBranchCommit, target, mainBranch) !=
      0) {
    std::cout << "system_git_repo_source::error while git get annotaded commit "
                 "(main): "
              << git_error_last()->message << std::endl;
    return false;
  }

  git_annotated_commit *their_heads[] = {lastMainBranchCommit};
  git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
  if (git_merge(target, (const git_annotated_commit **)(their_heads), 1,
                &merge_opts, &checkout_opts) != 0) {
    std::cout << "system_git_repo_source::error while git get annotaded commit "
                 "(main): "
              << git_error_last()->message << std::endl;
    return false;
  }

  return true;
}

bool system_git_repo_source::pull_changes()
{
  return pull_changes(repo);
}

bool system_git_repo_source::open_repo(std::string path_to_repo) {
  if (git_repository_open(&repo, path_to_repo.c_str()) != 0) {
    std::cout << "system_git_repo_source::error while git open(init path): "
              << git_error_last()->message << std::endl;
    return false;
  }
  return true;
}

bool system_git_repo_source::is_getted_path_git_repo(std::string path_to_repo) {
  if (git_repository_open_ext(NULL, path_to_repo.c_str(),
                              GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0) {
    return true;
  }
  return false;
}

bool system_git_repo_source::is_getted_initilaized_repo_submodule(git_submodule *submodule, git_repository ** repo_result)
{
  if (submodule == NULL){
    return false;
  }
  if (repo_result == nullptr){
    git_repository * local_repo;
    int result_code  = git_submodule_open(&local_repo, submodule);
    git_repository_free(local_repo);
    return result_code == 0;
  }
  return git_submodule_open(repo_result, submodule) == 0;
}

bool system_git_repo_source::fetch_repo(git_repository *target)
{
  git_remote *remote;
  if (git_remote_lookup(&remote, target, "origin") != 0) {
    show_last_git_error("system_git_repo_source::error while git remote lookup (origin): ");
    return false;
  }
  if (git_remote_fetch(remote, NULL, NULL, "fetch") != 0) {
    show_last_git_error( "system_git_repo_source::error while git remote fetch (origin): ");
    return false;
  }
  return true;

}

bool system_git_repo_source::fetch_repo()
{
  return fetch_repo(repo);
}

std::string system_git_repo_source::get_head_branch_name_for_repo(git_repository *target)
{
  std::string result;
  git_reference* head_reference;
    if (git_repository_head(&head_reference, target)!=0){
      show_last_git_error("system_git_repo_source: cant get HEAD branch name");
      return "";
    }
    result = git_reference_shorthand(head_reference);
    git_reference_free(head_reference);
    return result;
}

bool system_git_repo_source::is_submodule_of_repo(std::string path, git_submodule ** result_submodule)
{
  if (repo == NULL){
    return false;
  }
  if (result_submodule == nullptr){
    git_submodule * local_submodule;
    int result_code = git_submodule_lookup(&local_submodule, repo, path.c_str());
    git_submodule_free(local_submodule);
    return result_code == 0;
  }

  return git_submodule_lookup(result_submodule, repo, path.c_str()) == 0;
}

const char * REPO_MAIN_CONFIG_PATH = "repo.xml";
const char * REPO_THEME_FOLDER_POSTFIX = ".theme";

advanced_git_repo_source::advanced_git_repo_source(std::string repo_path, std::string repo_url):system_git_repo_source(repo_path,repo_url)
{
}

advanced_git_repo_source::~advanced_git_repo_source()
{
  xmlFreeDoc(xml_doc);
}

bool advanced_git_repo_source::load_themes_db()
{
  if (!is_getted_path_git_repo(path_to_folder)){
    if (!git_init_with_remote_origin(path_to_folder, repo_url)){
      return false;
    }
  }
  else{
    open_repo(path_to_folder);
  }
  if (!git_pull_specific_path(REPO_MAIN_CONFIG_PATH)){
    return false;
  }
  xml_doc = xmlReadFile((path_to_folder+ "/" + REPO_MAIN_CONFIG_PATH).c_str(), NULL, 0);
  themes_db = parse_themes_db_from_xml();
  return !themes_db.empty();
}

std::string advanced_git_repo_source::get_theme_path_for_alias(std::string alias)
{

  if (themes_db.count(alias) == 0){
    return "";
  }
  std::cout << "conky: using theme preset [" << alias << "]"<< std::endl;
  std::cout << "conky: pulling theme..." << std::endl;
  if (!git_pull_specific_path(alias + REPO_THEME_FOLDER_POSTFIX)){
    return "";
  }
  git_submodule * submodule;
  if (is_submodule_of_repo(alias + REPO_THEME_FOLDER_POSTFIX,&submodule)){
    git_repository * submodule_repo;
    if (is_getted_initilaized_repo_submodule(submodule, &submodule_repo)){
      std::cout << "conky: updating new submodule theme repo..." << std::endl;
      if (!pull_changes(submodule_repo,false)){
        return "";
      }
    }
    else{
      std::cout << "cloning submodule theme repo..." << std::endl;
      if (git_submodule_update(submodule, true, NULL) !=0){
        return "";
      }
    }
  }

  return path_to_folder + "/" + themes_db[alias];
}

const char * XML_REPO_INFO_NODE_NAME= "info";
const char * XML_REPO_INFO_NAME_PROP_NAME = "name";
const char * XML_REPO_INFO_DESCRIPTION_NODE_NAME = "description";
const char * XML_REPO_THEME_NODE_NAME = "theme";
const char * XML_REPO_THEME_ALIAS_PROP_NAME = "alias";
bool advanced_git_repo_source::set_repo_info_from_info_node(xmlNode * node){
  if (std::string((const char*)node->name) != XML_REPO_INFO_NODE_NAME){
    return false;
  }
  xmlChar * name_value =  xmlGetProp(node, (const xmlChar*)XML_REPO_INFO_NAME_PROP_NAME);
  if (name_value  == NULL){
    return false;
  }
  std::string local_repo_name = (const char *)name_value;
  if (!local_repo_name.empty()){
    repo_name = local_repo_name;
  }
  xmlFree(name_value);


  xmlNode * description_node = node->children;
  if (description_node == NULL){
    return false;
  }
  if (std::string((const char * )description_node->name) != XML_REPO_INFO_DESCRIPTION_NODE_NAME){
    return false;
  }

  xmlChar * description_value = xmlNodeGetContent(description_node);
  if (description_value == NULL){
    return false;
  }

  std::string local_repo_description = (const char *)description_value;
  if (!local_repo_description.empty()){
    repo_description = local_repo_description;
  }
  xmlFree(description_value);
  return true;
}
bool add_new_theme_from_theme_node_to_map(std::map<std::string,std::string> & add_to, xmlNode * node){
  if (std::string((const char*)node->name) != XML_REPO_THEME_NODE_NAME){
    return false;
  }
  xmlChar * alias_value =  xmlGetProp(node, (const xmlChar*)XML_REPO_THEME_ALIAS_PROP_NAME);
  if (alias_value == NULL){
    return false;
  }
  std::string alias = (const char *) alias_value;
  xmlFree(alias_value);
  if (alias.empty()){
    return false;
  }

  xmlChar * node_value = xmlNodeGetContent(node);
  if (node_value == NULL){
    return false;
  }
  std::string path = (const char *)node_value;
  xmlFree(node_value);
  if (path.empty()){
    return false;
  }
  add_to[alias] = path;
  return true;
}

std::map<std::string, std::string> advanced_git_repo_source::parse_themes_db_from_xml()
{
  std::map <std::string, std::string> result;
  if (xml_doc == NULL){
    std::cout << "advanced_git_repo_source::cant load xml main repo config file" << std::endl;
    return result;
  }

  xmlNode * root_element = xmlDocGetRootElement(xml_doc);
  for (xmlNode* node = root_element->children; node; node = node->next) {
    if (node->name == NULL){
      continue;
    }
    std::string node_name = (const char*)node->name;
    if (node_name  == XML_REPO_INFO_NODE_NAME){
      set_repo_info_from_info_node(node);
    }
    else if (node_name == XML_REPO_THEME_NODE_NAME){
      add_new_theme_from_theme_node_to_map(result, node);
    }
  }
  return result;
}

bool advanced_git_repo_source::git_init_with_remote_origin(std::string path_to_repo, std::string remote_origin_url)
{
  if (git_repository_init(&repo, path_to_repo.c_str(), false) != 0){
    show_last_git_error("advanced_git_repo_source: cant init repo");
    return false;
  }
  git_remote  * remote;
  if (git_remote_create(&remote, repo, "origin", remote_origin_url.c_str())!=0){
    show_last_git_error("advanced_git_repo_source: cant add remote origin");
    return false;
  }
  if (git_remote_fetch(remote, NULL, NULL, "fetch")!=0){
    show_last_git_error("advanced_git_repo_source: error while fetch");
    return false;
  }
  git_pull_specific_path(".gitmodules");
  git_repository_free(repo);
  open_repo(path_to_repo);
  return true;
}

bool advanced_git_repo_source::git_pull_specific_path(std::string path, bool use_base_branch)
{
  if (!fetch_repo()){
    return false;
  }
  git_object *target;
  std::string branch_name = BASE_REPO_BRANCH;
  if (!use_base_branch){
    branch_name = get_head_branch_name_for_repo(repo);
  }
  if (git_revparse_single(&target, repo, (std::string("origin/")+branch_name).c_str() ) != 0) {
    show_last_git_error("advanced_git_repo_source: cant find branch");
   return false;
  }
  git_checkout_options checkout_opts;
  git_checkout_options_init(&checkout_opts, GIT_CHECKOUT_OPTIONS_VERSION);
  checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;
  const char *paths[1] = {path.c_str()};
  checkout_opts.paths.strings = (char **)paths;
  checkout_opts.paths.count = 1;

  if (git_checkout_tree(repo, target, &checkout_opts) != 0) {
    show_last_git_error("advanced_git_repo_source: checkout failed");
    git_object_free(target);
    return false;
  }
  git_object_free(target);
  return true;
}



