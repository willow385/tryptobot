{ pkgs }: {
	deps = [
		pkgs.python38Full
    pkgs.gdb
    pkgs.vim
	];
  env = {
    LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
      # Neded for pandas / numpy
      pkgs.stdenv.cc.cc.lib
      pkgs.zlib
      # Needed for pygame
      pkgs.glib
    ];
    PYTHONBIN = "${pkgs.python38Full}/bin/python3.8";
  };
}