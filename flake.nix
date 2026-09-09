{
  description = "xpenguins - animated penguins on your X11 desktop";

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
          default = xpenguins;

          xpenguins = pkgs.stdenv.mkDerivation {
            pname = "xpenguins";
            version = "2.2";

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
          inputsFrom = [ self.packages.${system}.xpenguins ];
          nativeBuildInputs = with pkgs; [ cmake pkg-config ];
        };
      }
    );
}
