#!/bin/sh
#####################################
#
# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2022 Mikolaj Wielgus
# Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# https://www.gnu.org/licenses/gpl-3.0.html
# or you may search the http://www.gnu.org website for the version 3 license,
# or you may write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#
#####################################

display_help() {
    echo "Usage: $0 <ngspice repository>"
    echo "The necessary definition files will be copied from the Ngspice repository at <ngspice repository>"
}

if [ -z "$1" ]; then
    display_help
    exit
fi

NGSPICE_DIR="$1"
NGSPICE_DEVICES_DIR="$NGSPICE_DIR/src/spicelib/devices"

TARGET_DIR=$(dirname "$0")

# ADMS models not included for now.
FILES="\
    res/res.c             res/resdef.h             \
    cap/cap.c             cap/capdef.h             \
    ind/ind.c             ind/inddef.h             \
\
    tra/tra.c             tra/tradef.h             \
    ltra/ltra.c           ltra/ltradef.h           \
    urc/urc.c             urc/urcdef.h             \
    txl/txl.c             txl/txldef.h             \
\
    dio/dio.c             dio/diodef.h             \
    bjt/bjt.c             bjt/bjtdef.h             \
\
    jfet/jfet.c           jfet/jfetdef.h           \
    jfet2/jfet2.c         jfet2/jfet2def.h         \
\
    mes/mes.c             mes/mesdef.h             \
    mesa/mesa.c           mesa/mesadef.h           \
    hfet1/hfet1.c         hfet1/hfet1def.h         \
    hfet2/hfet2.c         hfet2/hfet2def.h
\
    mos1/mos1.c           mos1/mos1def.h           \
    mos2/mos2.c           mos2/mos2def.h           \
    mos3/mos3.c           mos3/mos3def.h           \
    bsim1/b1.c            bsim1/b1def.h            \
    bsim2/b2.c            bsim2/b2def.h            \
    mos6/mos6.c           mos6/mos6def.h           \
    bsim3/b3.c            bsim3/b3def.h            \
    mos9/mos9.c           mos9/mos9def.h           \
    bsimsoi/b4soi.c       bsimsoi/b4soidef.h       \
    bsim4/b4.c            bsim4/b4def.h            \
    bsim3soi_fd/b3soifd.c bsim3soi_fd/b3soifddef.h \
    bsim3soi_dd/b3soidd.c bsim3soi_dd/b3soidddef.h \
    bsim3soi_pd/b3soipd.c bsim3soi_pd/b3soipddef.h \
    hisim2/hsm2.c         hisim2/hsm2def.h         \
    hisimhv2/hsmhv2.c     hisimhv2/hsmhv2def.h     \
"

for f in $FILES; do
    mkdir -p "$TARGET_DIR/$(dirname $f)"
    echo "Copying $NGSPICE_DEVICES_DIR/$f to $TARGET_DIR/$f"
    cp "$NGSPICE_DEVICES_DIR/$f" "$TARGET_DIR/$f"
done
