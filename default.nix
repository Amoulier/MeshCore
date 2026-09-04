{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
  buildInputs = [
    pkgs.platformio
    pkgs.python3
    pkgs.gcc
    pkgs.gtest
  ];
}
