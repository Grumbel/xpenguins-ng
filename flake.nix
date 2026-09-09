{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    xpenguins_src.url = "http://xpenguins.seul.org/xpenguins-2.2.tar.gz";
    xpenguins_src.flake = false;
  };

  outputs = { self, nixpkgs, flake-utils, xpenguins_src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages = rec {
          default = xpenguins;

          xpenguins = pkgs.stdenv.mkDerivation rec {
            pname = "xpenguins";
            version = "2.2";

            src = xpenguins_src;

            patches = [
              ./fix-snprintf-error.diff
              ./fix-main.diff
              ./fix-XSetErrorHandler.diff
              ./fix-exit.diff
            ];

            buildInputs = with pkgs; [
              libx11
              libxpm
              libxt
              libxext
            ];
          };
        };
      }
    );
}
