FLTK_DIR=fltk-1.3.5
if [ ! -d ${FLTK_DIR} ]
then
  wget https://www.fltk.org/pub/fltk/1.3.5/fltk-1.3.5-source.tar.bz2
  tar -xjf fltk-1.3.5-source.tar.bz2
  (cd ${FLTK_DIR}; ./configure --disable-xft; make)
  rm fltk-1.3.5-source.tar.bz2
fi

