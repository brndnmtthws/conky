{
  description = "A Nix flake for Conky, including a dev shell";
  nixConfig = {
    substituters = [
      "https://conky.cachix.org"
    ];
    trusted-public-keys = [
      "conky.cachix.org-1:4H7kaqUIbxZO5LReWGyzwh3ktfYJb/K7E9au2gssyzM="
    ];
  };
  inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/nixos-unstable";
    };
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem
    (
      system: let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [self.overlay];
        };
      in
        with pkgs; rec
        {
          packages = flake-utils.lib.flattenTree {
            conky = conky;
          };

          defaultPackage = packages.conky;
          apps.conky = flake-utils.lib.mkApp {drv = packages.conky;};
          defaultApp = apps.conky;
          devShells.default = mkShell {
            buildInputs =
              packages.conky.buildInputs
              ++ packages.conky.nativeBuildInputs
              ++ [
                alejandra # for beautifying flake
                lefthook # for git hooks
                nodejs # for web/ stuff
                # for docs
                (python3.withPackages (ps: with ps; [jinja2]))
              ];
          };
        }
    )
    // {
      overlay = final: prev: {
        conky = with final;
          stdenv.mkDerivation rec {
            name = "conky";
            src = ./.;
            nativeBuildInputs = [
              clang_16
              cmake
              git
              llvmPackages_16.clang-unwrapped
              ninja
              pkg-config
            ];
            buildInputs =
              [
                freetype
                gettext
                imlib2
                llvmPackages_16.libcxx
                llvmPackages_16.libcxxabi
                lua5_4
                ncurses
                xorg.libICE
                xorg.libSM
                xorg.libX11
                xorg.libXdamage
                xorg.libXext
                xorg.libXfixes
                xorg.libXft
                xorg.libXinerama
              ]
              ++ lib.optional stdenv.isDarwin darwin.libobjc;
          };
      };
    };
}
