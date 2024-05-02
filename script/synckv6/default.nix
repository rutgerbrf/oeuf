{ pkgs ? import <nixpkgs> { } }: with pkgs;

stdenv.mkDerivation {
  name = "oeuf-synckv6";
  src = ./.;

  buildInputs = [ bash rclone ];
  nativeBuildInputs = [ makeWrapper ];
  installPhase = ''
    mkdir -p $out/bin
    cp oeuf-synckv6.sh $out/bin/oeuf-synckv6
    wrapProgram $out/bin/oeuf-synckv6 \
      --prefix PATH : ${lib.makeBinPath [ bash rclone ]}
  '';
}
