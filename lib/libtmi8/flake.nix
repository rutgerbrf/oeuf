{
  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOs/nixpkgs/*.tar.gz";
    flake-utils.url = "https://flakehub.com/f/numtide/flake-utils/0.1.88.tar.gz";
  };

  outputs = { self, nixpkgs, flake-utils, ... }@inputs:
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ ];
          };

          inherit (pkgs.gcc13) stdenv;

          oeuf-libtmi8 = stdenv.mkDerivation {
            name = "oeuf-libtmi8";
            src = pkgs.lib.cleanSource ./.;

            nativeBuildInputs = with pkgs; [ gcc13 ];
            buildInputs = with pkgs; [ arrow-cpp boost182 ];
            buildPhase = ''
              make libtmi8
            '';

            installPhase = ''
              make install DESTDIR="$out"
            '';
          };
        in
        {
          packages.oeuf-libtmi8 = oeuf-libtmi8;

          devShells.default = pkgs.mkShell {
            inputsFrom = [ oeuf-libtmi8 ];
          };

          formatter = pkgs.nixpkgs-fmt;
        });
}
