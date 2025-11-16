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
#ifndef THEMESPRESETMANAGER_H
#define THEMESPRESETMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <git2.h>

class AbstractThemesSource{
 public:
  AbstractThemesSource();
  virtual bool loadThemesDb() = 0; // or reload
  virtual std::map<std::string, std::string> getThemesDb() = 0;
  virtual std::vector<std::string> loadPossibleAliasOfThemes() = 0 ;
  virtual std::string getThemePathForAlias(std::string alias) = 0 ;
 protected:
  bool isDir(std::string dir);
  bool mkdir(std::string path);

};
class BaseLocalFileSystemFolderSource : public AbstractThemesSource
{
 public:
  BaseLocalFileSystemFolderSource(std::string pathToFolder);
  virtual bool loadThemesDb();
  virtual std::map<std::string, std::string> getThemesDb();
  virtual std::vector<std::string> loadPossibleAliasOfThemes();
  virtual std::string getThemePathForAlias(std::string alias);
 protected:
  const std::string _pathToFolder;
  std::string parseThemeAliasFromFilename(std::string filename);
  std::map<std::string, std::string>  _themesDb;
};
class RepoLocalFileSystemSource : public BaseLocalFileSystemFolderSource{
 public:
  RepoLocalFileSystemSource(std::string pathToRepo);
  virtual bool loadThemesDb();
  //virtual std::map<std::string, std::string> getThemesDb();
  //virtual std::vector<std::string> loadPossibleAliasOfThemes();
  //virtual std::string getThemePathForAlias(std::string alias);
 protected:
  std::string checkIsDirHaveConfig(std::string path);
};
class SystemGitRepoSource : public RepoLocalFileSystemSource{
 public:
  SystemGitRepoSource (std::string pathToNewGitRepo, std::string cloneUrlToGitRepo); // if repo exists url dont used simple pull if not git clone
  SystemGitRepoSource (std::string pathToGitAlreadyClonedRepo);
  ~SystemGitRepoSource();
  virtual bool loadThemesDb(); // if loded one or more theme returns true
 protected:
  bool pullChanges();
  bool openRepo(std::string pathToRepo);
  bool isGettedPathGitRepo(std::string pathToRepo);
  git_repository * _repo = NULL;
  // /std::string _pathToRepo = "";
  std::string _repoUrl = "";
  //   virtual std::map<std::string, std::string> getThemesDb();
//   virtual std::vector<std::string> loadPossibleAliasOfThemes();
//   virtual std::string getThemePathForAlias(std::string alias);

};

class ThemesPresetManager {
public:
  ThemesPresetManager(AbstractThemesSource * themesSource);
  std::vector<std::string> getPossiblePreloadThemeAlias();
  std::string getThemePath(std::string id);
private:
  AbstractThemesSource * _themesSource;
};



#endif  // THEMESPRESETMANAGER_H
