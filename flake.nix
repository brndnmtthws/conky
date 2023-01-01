{
  description = "A Nix flake for Conky";

  inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/nixos-unstable";
    };
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };

  outputs = { self, nixpkgs, flake-utils, ... }: flake-utils.lib.eachDefaultSystem
    (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ self.overlay ];
        };
      in
      rec
      {
        packages = flake-utils.lib.flattenTree {
          conky = pkgs.conky;
        };
        defaultPackage = packages.conky;
        apps.conky = flake-utils.lib.mkApp { drv = packages.conky; };
        defaultApp = apps.conky;
      }
    ) // {
    overlay = final: prev: {
      conky = with final; stdenv.mkDerivation rec {
        name = "conky";
        src = ./.;
        nativeBuildInputs = [
          clang_14
          cmake
          git
          ninja
          pkg-config
        ];
        buildInputs = [
          freetype
          gettext
          imlib2
          llvmPackages_14.libcxx
          llvmPackages_14.libcxxabi
          lua
          ncurses
          xorg.libICE
          xorg.libSM
          xorg.libX11
          xorg.libXext
          xorg.libXft
          xorg.libXinerama
        ];
      };
    };
  };
}
