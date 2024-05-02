{ pkgs ? import <nixpkgs> { } }: with pkgs;

stdenv.mkDerivation {
  name = "oeuf-archiver";
  src = ./.;

  buildInputs = [ bash rclone oeuf-bundleparquet ];
  nativeBuildInputs = [ makeWrapper ];
  installPhase = ''
    mkdir -p $out/bin
    cp oeuf-archiver.sh $out/bin/oeuf-archiver
    wrapProgram $out/bin/oeuf-archiver \
      --prefix PATH : ${lib.makeBinPath [ bash rclone oeuf-bundleparquet ]}
  '';
}
