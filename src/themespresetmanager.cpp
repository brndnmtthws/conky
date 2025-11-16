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
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <git2.h>
#include <git2/common.h>
const char * THEME_END_NAME = ".theme"; // can be changed at another one
const char * THEME_CONFIG_END = ".conf"; // can be changed at another one

std::string getStdoutFromCommand(std::string cmd) { // deprecated, need if use system and need to working with bash stdoutd

  std::string data;
  FILE * stream;
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



ThemesPresetManager::ThemesPresetManager(AbstractThemesSource *themesSource):_themesSource(themesSource) {
}

std::vector<std::string> ThemesPresetManager::getPossiblePreloadThemeAlias()
{
  return _themesSource->loadPossibleAliasOfThemes();
}

std::string ThemesPresetManager::getThemePath(std::string id)
{
  return _themesSource->getThemePathForAlias(id);
}

AbstractThemesSource::AbstractThemesSource()
{

}

bool AbstractThemesSource::isDir(std::string path)
{
  struct stat info;
  if (stat( path.c_str(), &info ) == 0){
    if(!(info.st_mode & S_IFDIR ))  {
      return false;
    }
  }
  else{
    return false;
  }
  return true;
}

bool AbstractThemesSource::mkdir(std::string path)
{
  return std::filesystem::create_directory(path);
}


BaseLocalFileSystemFolderSource::BaseLocalFileSystemFolderSource(std::string pathToFolder):AbstractThemesSource(),_pathToFolder(pathToFolder)
{
}

bool BaseLocalFileSystemFolderSource::loadThemesDb()
{
  if (!isDir(_pathToFolder)){
    return false;
  }



  for (const auto & entry : std::filesystem::directory_iterator(_pathToFolder)){

    std::string themeAlias = parseThemeAliasFromFilename(entry.path());
    if (themeAlias != ""){
      _themesDb[themeAlias] = entry.path();
    }
  }
  return !_themesDb.empty();
}

std::map<std::string, std::string> BaseLocalFileSystemFolderSource::getThemesDb()
{
  return _themesDb;
}

std::vector<std::string> BaseLocalFileSystemFolderSource::loadPossibleAliasOfThemes()
{
  std::vector<std::string> result;
  for (std::pair<std::string,std::string> theme : _themesDb){
    result.push_back(theme.first);
  }
  return result;
}

std::string BaseLocalFileSystemFolderSource::getThemePathForAlias(std::string alias)
{
  if(_themesDb.empty() | !_themesDb.count(alias)){
    return "";
  }
  else{
    return _themesDb[alias];
  }
}

std::string BaseLocalFileSystemFolderSource::parseThemeAliasFromFilename(std::string filename)
{
  if (filename.find(THEME_END_NAME)==filename.npos){
    std::cout << "BaseLocalFileSystemFolderSource. cant parse aloas from filename" << std::endl;
    return "";
  }
  if (filename.rfind("/")!=filename.npos){
    filename = filename.substr(filename.rfind("/") +1, filename.size());
   // std::cout << filename << std::endl;
  }
  return filename.substr(0,filename.find(THEME_END_NAME));
}

RepoLocalFileSystemSource::RepoLocalFileSystemSource(std::string pathToRepo):BaseLocalFileSystemFolderSource(pathToRepo)
{
}

bool RepoLocalFileSystemSource::loadThemesDb()
{

  if (!isDir(_pathToFolder)){
    return false;
  }
  for(const auto & entry : std::filesystem::directory_iterator(_pathToFolder)){
    std::string configPath = checkIsDirHaveConfig(entry.path());
    if  (configPath != ""){
      _themesDb[parseThemeAliasFromFilename(entry.path())] = configPath;
    }
  }
  if (_themesDb.empty()){
    return false;
  }
  return true;

}


std::string RepoLocalFileSystemSource::checkIsDirHaveConfig(std::string path)
{
  if (!isDir(path)){
    return "";
  }
  for (const auto & entry : std::filesystem::directory_iterator(path)){

    std::string filename = entry.path();
    filename = filename.substr(filename.rfind("/") +1, filename.size()); // remove dirs and get simply filename
    if (filename.find(THEME_CONFIG_END)!=filename.npos){
      return entry.path();
    }
  }
  return ""; // cant find config

}

SystemGitRepoSource::SystemGitRepoSource(std::string  pathToNewGitRepo, std::string cloneUrlToGitRepo):RepoLocalFileSystemSource(pathToNewGitRepo),_repoUrl(cloneUrlToGitRepo)
{
  git_libgit2_init();
}

SystemGitRepoSource::SystemGitRepoSource(std::string pathToGitAlreadyClonedRepo):RepoLocalFileSystemSource(pathToGitAlreadyClonedRepo)
{
  git_libgit2_init();
}

SystemGitRepoSource::~SystemGitRepoSource()
{
  git_libgit2_shutdown();
}

bool SystemGitRepoSource::loadThemesDb()
{
  if (!isDir(_pathToFolder)){
    if (!mkdir(_pathToFolder)){
      std::cout << "SystemGitRepoSource::path dir repo not exists and cant create" << std::endl;
      return false;
    }
  }
  if (isGettedPathGitRepo(_pathToFolder)){
    if (openRepo(_pathToFolder)){
      if (pullChanges()){
        return RepoLocalFileSystemSource::loadThemesDb();
      }
      return false;
    }
    return false;
  }
  else{
    if (_repoUrl != ""){
      std::cout << "cloning" << std::endl;
      if (git_clone(&_repo, _repoUrl.c_str(), _pathToFolder.c_str(), NULL)!=0){
        std::cout << "SystemGitRepoSource::error while git clone(init url): "<<git_error_last()->message << std::endl;
        return false;
      }
      return RepoLocalFileSystemSource::loadThemesDb();
    }
  }
  return false;


}

bool SystemGitRepoSource::pullChanges() // maybe need to change this
{
  git_remote *remote;
  if(git_remote_lookup(&remote, _repo, "origin")!=0){
    std::cout << "SystemGitRepoSource::error while git remote lookup (origin): "<<git_error_last()->message << std::endl;
    return false;
  }
  if (git_remote_fetch(remote,NULL,NULL,"fetch")!=0){
    std::cout << "SystemGitRepoSource::error while git remote fetch (origin): "<<git_error_last()->message << std::endl;
    return false;
  }
  git_reference * mainBranch = nullptr;
  if (git_reference_lookup(&mainBranch, _repo, "refs/remotes/origin/main")!=0){
     std::cout << "SystemGitRepoSource::error while git lookup (main): "<<git_error_last()->message << std::endl;
    return false;
  }
  git_annotated_commit * lastMainBranchCommit = nullptr;
  if (git_annotated_commit_from_ref(&lastMainBranchCommit, _repo, mainBranch)!=0){
    std::cout << "SystemGitRepoSource::error while git get annotaded commit (main): "<<git_error_last()->message << std::endl;
    return false;
  }

  git_annotated_commit* their_heads[] = { lastMainBranchCommit};
  git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
  git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
  if (git_merge(_repo, (const git_annotated_commit**)(their_heads), 1, &merge_opts, &checkout_opts)!=0){
    std::cout << "SystemGitRepoSource::error while git get annotaded commit (main): "<<git_error_last()->message << std::endl;
    return false;
  }

  return true;

}

bool SystemGitRepoSource::openRepo(std::string pathToRepo)
{
  if (git_repository_open(&_repo, pathToRepo.c_str())!=0){
    std::cout << "SystemGitRepoSource::error while git open(init path): "<<git_error_last()->message << std::endl;
    return false;
  }
  return true;

}

bool SystemGitRepoSource::isGettedPathGitRepo(std::string pathToRepo)
{
  if (git_repository_open_ext(
          NULL, pathToRepo.c_str(), GIT_REPOSITORY_OPEN_NO_SEARCH, NULL) == 0) {
    return true;
  }
  return false;
}
