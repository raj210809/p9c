{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    gnumake
    gdb
  ];

  shellHook = ''
    echo "📦 libixp dev shell ready"
  '';
}
