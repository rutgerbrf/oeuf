{
  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOs/nixpkgs/*.tar.gz";
    flake-utils.url = "https://flakehub.com/f/numtide/flake-utils/0.1.88.tar.gz";
    libtmi8 = {
      url = "path:./lib/libtmi8";
      inputs.nixpkgs.follows = "nixpkgs";
      inputs.flake-utils.follows = "flake-utils";
    };
  };

  outputs = { self, nixpkgs, flake-utils, libtmi8, ... }@inputs:
    {
      nixosModules = rec {
        oeuf = import ./module self;
        default = oeuf;
      };
    } // flake-utils.lib.eachDefaultSystem
      (system:
        let
          libtmi8Overlay = final: prev: { oeuf-libtmi8 = libtmi8.packages.${system}.oeuf-libtmi8; };

          pkgs = import nixpkgs {
            inherit system;
            overlays = [ libtmi8Overlay ];
          };
          boostPkg = pkgs.boost182;

          inherit (pkgs.gcc13) stdenv;

          oeuf-augmentkv6 = stdenv.mkDerivation {
            name = "oeuf-augmentkv6";
            src = ./.;

            nativeBuildInputs = with pkgs; [ gcc13 boostPkg ];
            buildInputs = with pkgs; [ arrow-cpp oeuf-libtmi8 ];
            buildPhase = ''
              cd src/augmentkv6
              make augmentkv6
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp augmentkv6 $out/bin/oeuf-augmentkv6
            '';
          };

          oeuf-filterkv6 = stdenv.mkDerivation {
            name = "oeuf-filterkv6";
            src = ./.;

            nativeBuildInputs = with pkgs; [ gcc13 ];
            buildInputs = with pkgs; [ arrow-cpp oeuf-libtmi8 ];
            buildPhase = ''
              cd src/filterkv6
              make filterkv6
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp filterkv6 $out/bin/oeuf-filterkv6
            '';
          };

          oeuf-bundleparquet = stdenv.mkDerivation {
            name = "oeuf-bundleparquet";
            src = ./.;

            nativeBuildInputs = with pkgs; [ gcc13 ];
            buildInputs = with pkgs; [ arrow-cpp curl nlohmann_json prometheus-cpp zlib oeuf-libtmi8 ];
            buildPhase = ''
              cd src/bundleparquet
              make bundleparquet
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp bundleparquet $out/bin/oeuf-bundleparquet
            '';
          };

          oeuf-querykv1 = stdenv.mkDerivation {
            name = "oeuf-querykv1";
            src = ./.;

            nativeBuildInputs = with pkgs; [ gcc13 ];
            buildInputs = with pkgs; [ oeuf-libtmi8 boostPkg ];
            buildPhase = ''
              cd src/querykv1
              make querykv1
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp querykv1 $out/bin/oeuf-querykv1
            '';
          };

          oeuf-recvkv6 = stdenv.mkDerivation {
            name = "oeuf-recvkv6";
            src = ./.;

            nativeBuildInputs = with pkgs; [ gcc13 ];
            buildInputs = with pkgs; [ zeromq zlib arrow-cpp nlohmann_json prometheus-cpp rapidxml oeuf-libtmi8 ];
            buildPhase = ''
              cd src/recvkv6
              make recvkv6
            '';

            installPhase = ''
              mkdir -p $out/bin
              cp recvkv6 $out/bin/oeuf-recvkv6
            '';
          };

          oeuf-archiver = import ./script/archiver {
            pkgs = pkgs // { inherit oeuf-bundleparquet; };
          };

          oeuf-synckv6 = import ./script/synckv6 { inherit pkgs; };
        in
        {
          packages.oeuf-archiver = oeuf-archiver;
          packages.oeuf-augmentkv6 = oeuf-augmentkv6;
          packages.oeuf-synckv6 = oeuf-synckv6;
          packages.oeuf-filterkv6 = oeuf-filterkv6;
          packages.oeuf-bundleparquet = oeuf-bundleparquet;
          packages.oeuf-querykv1 = oeuf-querykv1;
          packages.oeuf-recvkv6 = oeuf-recvkv6;

          devShells.default = pkgs.mkShell {
            inputsFrom = [ oeuf-bundleparquet oeuf-querykv1 oeuf-recvkv6 ];
            buildInputs = with pkgs; [ xercesc xsd ];
          };

          formatter = pkgs.nixpkgs-fmt;
        });
}
