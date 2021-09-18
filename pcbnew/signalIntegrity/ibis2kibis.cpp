#include "ibis2kibis.h"

bool convertKibisFile( IbisParser* aParser, KIBIS_FILE* aFile )
{
    bool status = true;

    if( aFile == nullptr || aParser == nullptr )
    {
        std::cerr << "Internal Error: Ibis -> Kibis, a pointer is nullptr" << std::endl;
        status = false;
    }
    else
    {
        aFile->m_fileName = aParser->m_ibisFile->m_header.m_fileName;
        aFile->m_fileRev = aParser->m_ibisFile->m_header.m_fileRevision;
        aFile->m_ibisVersion = aParser->m_ibisFile->m_header.m_ibisVersion;
        aFile->m_date = aParser->m_ibisFile->m_header.m_date;
        aFile->m_notes = aParser->m_ibisFile->m_header.m_notes;
        aFile->m_disclaimer = aParser->m_ibisFile->m_header.m_disclaimer;
        aFile->m_copyright = aParser->m_ibisFile->m_header.m_copyright;
    }

    return status;
}

bool convertKibisModel( IbisParser* aParser, IbisModel* aSource, KIBIS_MODEL* aDest )
{
    bool status = true;

    if( aSource == nullptr || aDest == nullptr || aParser == nullptr )
    {
        std::cerr << "Internal Error: Ibis -> Kibis, a pointer is nullptr" << std::endl;
        status = false;
    }
    else
    {
        aDest->m_name = aSource->m_name;
        aDest->m_type = aSource->m_type;

        aDest->m_description = wxString( "No description available." );

        for( IbisModelSelector modelSelector : aParser->m_ibisFile->m_modelSelectors )
        {
            for( IbisModelSelectorEntry entry : modelSelector.m_models )
            {
                if( entry.m_modelName == aDest->m_name )
                {
                    aDest->m_description = entry.m_modelDescription;
                }
            }
        }


        aDest->m_vinh = aSource->m_vinh;
        aDest->m_vinl = aSource->m_vinl;
        aDest->m_vref = aSource->m_vref;
        aDest->m_rref = aSource->m_rref;
        aDest->m_cref = aSource->m_cref;
        aDest->m_vmeas = aSource->m_vmeas;

        aDest->m_enable = aSource->m_enable;
        aDest->m_polarity = aSource->m_polarity;

        aDest->m_ramp = aSource->m_ramp;
        aDest->m_risingWaveforms = aSource->m_risingWaveforms;
        aDest->m_fallingWaveforms = aSource->m_fallingWaveforms;
        aDest->m_GNDClamp = aSource->m_GNDClamp;
        aDest->m_GNDClampReference = aSource->m_GNDClampReference;
        aDest->m_POWERClamp = aSource->m_POWERClamp;
        aDest->m_POWERClampReference = aSource->m_POWERClampReference;

        aDest->m_C_comp = aSource->m_C_comp;
        aDest->m_voltageRange = aSource->m_voltageRange;
        aDest->m_temperatureRange = aSource->m_temperatureRange;
        aDest->m_pullupReference = aSource->m_pullupReference;
        aDest->m_pulldownReference = aSource->m_pulldownReference;

        aDest->m_Rgnd = aSource->m_Rgnd;
        aDest->m_Rpower = aSource->m_Rpower;
        aDest->m_Rac = aSource->m_Rac;
        aDest->m_Cac = aSource->m_Cac;
        aDest->m_pullup = aSource->m_pullup;
        aDest->m_pulldown = aSource->m_pulldown;
    }
    return status;
}


bool convertKibisComponent( IbisParser* aParser, std::vector<KIBIS_MODEL*> aModels,
                            IbisComponent* aSource, KIBIS_COMPONENT* aDest )
{
    bool status = true;

    aDest->m_name = aSource->m_name;
    aDest->m_manufacturer = aSource->m_manufacturer;
    wxString m_name;
    /** @brief Name of the manufacturer */
    wxString m_manufacturer;

    std::vector<KIBIS_PIN*> m_pins;
    for( IbisComponentPin iPin : aSource->m_pins )
    {
        KIBIS_PIN* kPin = new KIBIS_PIN();
        kPin->m_signalName = iPin.m_signalName;
        kPin->m_pinNumber = iPin.m_pinName;

        kPin->R_pin = aSource->m_package.m_Rpkg;
        kPin->L_pin = aSource->m_package.m_Lpkg;
        kPin->C_pin = aSource->m_package.m_Cpkg;

        // The values listed in the [Pin] description section override the default
        // values defined in [Package]

        // @TODO : Reading the IBIS standard, I can't figure out if we are supposed
        // to replace typ, min, and max, or just the typ ?

        if( !std::isnan( iPin.m_Rpin ) )
        {
            kPin->R_pin.value[IBIS_CORNER::TYP] = iPin.m_Rpin;
            kPin->R_pin.value[IBIS_CORNER::MIN] = iPin.m_Rpin;
            kPin->R_pin.value[IBIS_CORNER::MAX] = iPin.m_Rpin;
        }
        if( !std::isnan( iPin.m_Lpin ) )
        {
            kPin->L_pin.value[IBIS_CORNER::TYP] = iPin.m_Lpin;
            kPin->L_pin.value[IBIS_CORNER::MIN] = iPin.m_Lpin;
            kPin->L_pin.value[IBIS_CORNER::MAX] = iPin.m_Lpin;
        }
        if( !std::isnan( iPin.m_Cpin ) )
        {
            kPin->C_pin.value[IBIS_CORNER::TYP] = iPin.m_Cpin;
            kPin->C_pin.value[IBIS_CORNER::MIN] = iPin.m_Cpin;
            kPin->C_pin.value[IBIS_CORNER::MAX] = iPin.m_Cpin;
        }

        bool                  modelSelected = false;
        std::vector<wxString> listOfModels;

        for( IbisModelSelector modelSelector : aParser->m_ibisFile->m_modelSelectors )
        {
            if( modelSelector.m_name == iPin.m_modelName )
            {
                for( IbisModelSelectorEntry model : modelSelector.m_models )
                {
                    listOfModels.push_back( model.m_modelName );
                }
                modelSelected = true;
                break;
            }
        }
        if( !modelSelected )
        {
            listOfModels.push_back( iPin.m_modelName );
        }

        for( wxString modelName : listOfModels )
        {
            for( KIBIS_MODEL* model : aModels )
            {
                if( model->m_name == modelName )
                {
                    kPin->m_models.push_back( model );
                }
            }
        }

        aDest->m_pins.push_back( kPin );
    }

    return status;
}


bool convertKibisAll( IbisParser* aParser, std::vector<KIBIS_COMPONENT*>* aDest )
{
    bool status = true;

    KIBIS_FILE* kFile = new KIBIS_FILE();
    status &= convertKibisFile( aParser, kFile );

    std::vector<KIBIS_MODEL*>     kModels;
    std::vector<KIBIS_COMPONENT*> kComponents;

    //
    for( IbisModel iModel : aParser->m_ibisFile->m_models )
    {
        KIBIS_MODEL* kModel = new KIBIS_MODEL();
        kModel->m_file = kFile;
        status &= convertKibisModel( aParser, &iModel, kModel );
        kModels.push_back( kModel );
    }

    for( IbisComponent iComponent : aParser->m_ibisFile->m_components )
    {
        KIBIS_COMPONENT* kComponent = new KIBIS_COMPONENT();
        kComponent->m_file = kFile;
        status &= convertKibisComponent( aParser, kModels, &iComponent, kComponent );
        kComponents.push_back( kComponent );
    }


    *aDest = kComponents;

    return status;
}