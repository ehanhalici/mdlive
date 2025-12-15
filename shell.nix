{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "mdlive-dev-env";

  nativeBuildInputs = with pkgs; [
    gcc
    cmake
    gnumake
    pkg-config
    git
    autoconf
  ];

  # 2. Sistem Kütüphaneleri (FLTK bunlara bağlanacak)
  buildInputs = with pkgs; [
    xorg.libX11
    xorg.libXext
    xorg.libXft       # Font render
    xorg.libXinerama  # Çoklu monitör
    xorg.libXcursor   # Mouse ikonları
    xorg.libXrender   # Şeffaflık vb.
    xorg.libXfixes    # X11 düzeltmeleri
      
    # OpenGL
    libglvnd
    libGLU
    iconv    
    # Font Konfigürasyonu
    fontconfig
  ];

  shellHook = ''
    echo "Derleme Ortamı Hazır"
  '';
}
