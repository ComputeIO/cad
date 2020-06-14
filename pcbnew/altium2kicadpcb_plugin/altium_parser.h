/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ALTIUM_PARSER_H
#define ALTIUM_PARSER_H

#include <map>
#include <memory>

#include <math/util.h>
#include <wx/gdicmn.h>


namespace CFB
{
class CompoundFileReader;
struct COMPOUND_FILE_ENTRY;
} // namespace CFB

// Helper method to find file inside compound file
const CFB::COMPOUND_FILE_ENTRY* FindStream(
        const CFB::CompoundFileReader& aReader, const char* aStreamName );


class ALTIUM_PARSER
{
public:
    ALTIUM_PARSER( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry );
    ~ALTIUM_PARSER() = default;

    template <typename Type>
    Type Read()
    {
        if( GetRemainingBytes() >= sizeof( Type ) )
        {
            Type val = *(Type*) ( m_pos );
            m_pos += sizeof( Type );
            return val;
        }
        else
        {
            m_error = true;
            return 0;
        }
    }

    wxString ReadWxString()
    {
        uint8_t len = Read<uint8_t>();
        if( GetRemainingBytes() >= len )
        {

            //altium uses LATIN1/ISO 8859-1, convert it
            wxString val = wxString( m_pos, wxConvISO8859_1, len );
            m_pos += len;
            return val;
        }
        else
        {
            m_error = true;
            return wxString( "" );
        }
    }

    int32_t ReadKicadUnit()
    {
        return ConvertToKicadUnit( Read<int32_t>() );
    }

    int32_t ReadKicadUnitX()
    {
        return ReadKicadUnit();
    }

    int32_t ReadKicadUnitY()
    {
        return -ReadKicadUnit();
    }

    wxPoint ReadWxPoint()
    {
        int32_t x = ReadKicadUnitX();
        int32_t y = ReadKicadUnitY();
        return { x, y };
    }

    wxSize ReadWxSize()
    {
        int32_t x = ReadKicadUnit();
        int32_t y = ReadKicadUnit();
        return { x, y };
    }

    size_t ReadAndSetSubrecordLength()
    {
        uint32_t length = Read<uint32_t>();
        m_subrecord_end = m_pos + length;
        return length;
    }

    std::map<wxString, wxString> ReadProperties();

   static int32_t ConvertToKicadUnit( const int32_t aValue )
   {
      int64_t tmp_aValue,rnd_aValue;

      tmp_aValue = ((int64_t) aValue ) * 254L;
      rnd_aValue = (tmp_aValue < 0) ? (tmp_aValue - 199) : (tmp_aValue + 199);

      rnd_aValue = rnd_aValue / 400;


      return ( (int32_t)(rnd_aValue * 4) );
   }

    static int32_t ConvertToKicadUnit( const double aValue )
    {
      int64_t tmp_aValue,rnd_aValue;

      tmp_aValue = (int64_t) (aValue  * 254);
      rnd_aValue = (tmp_aValue < 0) ? (tmp_aValue - 50) : (tmp_aValue + 50);

      rnd_aValue = rnd_aValue / 100;

      return ( (int32_t)(rnd_aValue) );
    }

    static int PropertiesReadInt(
            const std::map<wxString, wxString>& aProperties, const wxString& aKey, int aDefault );

    static double PropertiesReadDouble( const std::map<wxString, wxString>& aProperties,
            const wxString& aKey, double aDefault );

    static bool PropertiesReadBool(
            const std::map<wxString, wxString>& aProperties, const wxString& aKey, bool aDefault );

    static int32_t PropertiesReadKicadUnit( const std::map<wxString, wxString>& aProperties,
            const wxString& aKey, const wxString& aDefault );

    static wxString PropertiesReadString( const std::map<wxString, wxString>& aProperties,
            const wxString& aKey, const wxString& aDefault );

    void Skip( size_t aLength )
    {
        if( GetRemainingBytes() >= aLength )
        {
            m_pos += aLength;
        }
        else
        {
            m_error = true;
        }
    }

    void SkipSubrecord()
    {
        if( m_subrecord_end == nullptr || m_subrecord_end < m_pos )
        {
            m_error = true;
        }
        else
        {
            m_pos = m_subrecord_end;
        }
    };

    size_t GetRemainingBytes() const
    {
        return m_pos == nullptr ? 0 : m_size - ( m_pos - m_content.get() );
    }

    size_t GetRemainingSubrecordBytes() const
    {
        return m_pos == nullptr || m_subrecord_end == nullptr || m_subrecord_end <= m_pos ?
                       0 :
                       m_subrecord_end - m_pos;
    };

    bool HasParsingError()
    {
        return m_error;
    }

private:
    std::unique_ptr<char[]> m_content;
    size_t                  m_size;

    char* m_pos;           // current read pointer
    char* m_subrecord_end; // pointer which points to next subrecord start
    bool  m_error;
};


#endif //ALTIUM_PARSER_H
