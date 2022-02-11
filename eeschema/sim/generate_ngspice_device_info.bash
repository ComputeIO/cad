#!/bin/bash

run_ngspice_command()
{
    ngspice -n 2>/dev/null << END
        *
        .control
        $1
        .endc
END
}

{
    echo "#include <sim/ngspice.h>"
    echo "const std::map<wxString, NGSPICE::DEVICE_INFO> NGSPICE::DevicesInfo = {"

    run_ngspice_command "devhelp" | while read -r model_name sep model_description
    do
        if [ -n "$model_name" ] \
            && [ "$sep" = ":" ] \
            && [ -n "$model_description" ]
        then
            echo "    { \"${model_name,,}\", { \"$model_description\","
        
            run_ngspice_command "devhelp $model_name" | while read -r id name dir description
            do
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

            echo "        }"
            echo "    } },"
        fi
    done

    echo "};"
} > $(dirname "$0")/ngspice_devices.cpp
