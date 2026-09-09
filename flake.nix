{
  description = "xpenguins-ng - animated penguins on modern X11 desktops";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages = rec {
          default = xpenguins-ng;

          xpenguins-ng = pkgs.stdenv.mkDerivation {
            pname = "xpenguins-ng";
            version = "3.0";

            src = pkgs.lib.cleanSource ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
            ];

            buildInputs = with pkgs; [
              libx11
              libxpm
              libxext
              libxfixes
            ];
          };
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.xpenguins-ng ];
          nativeBuildInputs = with pkgs; [ cmake pkg-config ];
        };
      }
    );
}
