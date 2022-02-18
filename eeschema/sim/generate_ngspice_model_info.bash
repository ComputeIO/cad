#!/bin/bash

# This script is disgusting. Please rewrite it in Python.

run_ngspice_command()
{
    ngspice -n 2>/dev/null << END
        *
        .control
        $1
        .endc
END
}


MODELS=$(cat << END
    resistor  R  0  0
    capacitor C  0  0
    inductor  L  0  0

    ltra      O  0  0
    tranline  T  0  0
    urc       U  0  0
    transline Y  0  0

    diode     D  0  0

    bjt       Q  1  0
    vbic      Q  4  0
    hicum2    Q  8  0

    jfet      J  1  0
    jfet2     J  2  0

    mes       Z  1  0
    mesa      Z  2  0

    hfet1     Z  5  0
    hfet2     Z  6  0

    mos1      M  1  0
    mos2      M  2  0
    mos3      M  3  0
    bsim1     M  4  0
    bsim2     M  5  0
    mos6      M  6  0
    bsim3     M  8  0
    mos9      M  9  0
    b4soi     M  10 0
    bsim4     M  14 0

    b3soifd   M  55 0
    b3soidd   M  56 0
    b3soipd   M  57 0

    hisim2    M  68 0
    hisimhv1  M  73 1
    hisimhv2  M  73 2
END
)

{
    echo "// Generated using the $(basename $0) script."
    echo "// Modify that script instead of this file if you want to make changes."
    echo ""
    echo "#include <sim/ngspice.h>"
    echo ""

    echo "enum class NGSPICE::MODEL_TYPE"
    echo "{"

    echo "$MODELS" | while read -r model_name model_primitive model_level model_version; do
        if [ -n "$model_name" ]; then
            echo "    ${model_name^^},"
        fi
    done

    echo "};"
    echo ""

    echo "NGSPICE::MODEL_INFO NGSPICE::GetModelInfo( NGSPICE::MODEL_TYPE aType )"
    echo "{"
    echo "    switch( aType )"
    echo "    {"

    echo "$MODELS" | while read -r model_name model_primitive model_level model_version; do
        if [ -n "$model_name" ]; then
            run_ngspice_command "devhelp $model_name" | while read -r name sep description; do
                if [ "$sep" = "-" ]; then
                    echo "    case NGSPICE::MODEL_TYPE::${model_name^^}: return { \"$name\", \"$description\","
                fi
            done

            run_ngspice_command "devhelp $model_name" | while read -r id name dir description; do
                if [ "$id" = "Model" ] && [ "$name" = "Parameters" ]; then
                    echo "        // Model parameters"
                    echo "        {"
                elif [ "$id" = "Instance" ] && [ "$name" = "Parameters" ]; then
                    echo "        },"
                    echo "        // Instance parameters"
                    echo "        {"
                elif [ "$id" -eq "$id" ] 2>/dev/null \
                    && [ -n "$name" ] \
                    && [ -n "$dir" ] \
                    && [ -n "$description" ]
                then
                    echo "            { \"${name,,}\", { $id, NGSPICE::PARAM_DIR::${dir^^}, \"$description\" } },"
                fi
            done

            echo "        } };"
        fi
    done

    echo "    }"
    echo ""

    echo "    wxFAIL;"
    echo "    return {};"
    echo "}"
} > $(dirname "$0")/ngspice_models.cpp

#    run_ngspice_command "devhelp" | while read -r model_name sep model_description
#    do
#        if [ -n "$model_name" ] \
#            && [ "$sep" = ":" ] \
#            && [ -n "$model_description" ]
#        then
#            echo "    ${model_name^^},"
#        fi
#    done
#
#    run_ngspice_command "devhelp" | while read -r model_name sep model_description
#    do
#        if [ -n "$model_name" ] \
#            && [ "$sep" = ":" ] \
#            && [ -n "$model_description" ]
#        then
#            echo "    case NGSPICE::DEVICE_TYPE::${model_name^^}: return { \"${model_name,,}\", \"$model_description\","
#        
#            run_ngspice_command "devhelp $model_name" | while read -r id name dir description
#            do
#                if [ "$id" = "Model" ] && [ "$name" = "Parameters" ]; then
#                    echo "        // Model parameters"
#                    echo "        {"
#                elif [ "$id" = "Instance" ] && [ "$name" = "Parameters" ]; then
#                    echo "        },"
#                    echo "        // Instance parameters"
#                    echo "        {"
#                elif [ "$id" -eq "$id" ] 2>/dev/null \
#                    && [ -n "$name" ] \
#                    && [ -n "$dir" ] \
#                    && [ -n "$description" ]
#                then
#                    echo "            { \"${name,,}\", { $id, NGSPICE::PARAM_DIR::${dir^^}, \"$description\" } },"
#                fi
#            done
#
#            echo "        } };"
#        fi
#    done
