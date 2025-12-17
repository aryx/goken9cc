# Setup a dev environment for goken9cc using Nix.
# Run 'nix-shell --pure' from the root of the project to get a dev environment
# ready to compile/test/run goken from Linux or macOS (on amd64 or arm64).
# See also .github/workflows/nix.yml for its use in Github Actions (GHA).
# See also Dockerfile.
# See also my xix project and its shell.nix for more info about Nix.

let
   # fetch a specific nixos version for better reproducibility
   nixpkgs = fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-24.05";
   pkgs = import nixpkgs { config = {}; overlays = []; };
 in

pkgs.mkShell {
  packages = with pkgs; [
    # usually installed by default but does not hurt to add
    gcc binutils gnumake bash bison ed
    # old: 'glibc' but then does not work on macOS?
    
    # Optional utilities for development/debugging, ex: 'which'
    # Implicit utilities and libs installed by default in nix
    # (see "echo $PATH | sed -e 's/:/\n/g'" and "ldd ./bin/hello-world"):
    # - gnumake bash
    # - binutils gcc/clang glibc linux-headers gnu-config update-autotools
    # - coreutils findutils diffutils file gnugrep gnused
    # - gnutar gzip bzip2 unzip xz brotli zlib zstd curl
    # - not really needed but here: ed gawk patch patchelf
  ];    
  
  #coupling: Dockerfile
  shellHook = ''
    echo "Shell initialized for ${pkgs.stdenv.hostPlatform.system}"
    echo "you can now run:"
    echo "    ./configure"
    echo "    make"
    echo "    make install"
  '';
}
