{
  description = "A Nix flake for Conky, including a dev shell";
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
          overlays = [self.overlays.default];
        };
      in
        with pkgs; rec
        {
          packages = flake-utils.lib.flattenTree {
            conky = conky;
            default = conky;
          };

          apps.conky = flake-utils.lib.mkApp {drv = packages.conky;};
          apps.default = apps.conky;
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
      overlays.default = final: prev: {
        conky = with final;
          stdenv.mkDerivation rec {
            name = "conky";
            src = ./.;
            cmakeFlags = [
              "-DBUILD_LUA_CAIRO=ON"
              "-DBUILD_LUA_IMLIB2=ON"
              "-DBUILD_LUA_RSVG=ON"
            ];
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
                cairo
                freetype
                gettext
                imlib2
                librsvg
                llvmPackages_16.libcxx
                llvmPackages_16.libcxxabi
                lua5_4
                ncurses
                xorg.libICE
                xorg.libSM
                xorg.libX11
                xorg.libxcb
                xorg.libXdamage
                xorg.libXext
                xorg.libXfixes
                xorg.libXft
                xorg.libXi
                xorg.libXinerama
                xorg.xcbutilerrors
              ]
              ++ lib.optional stdenv.isDarwin darwin.libobjc;
          };
      };
    };
}
