/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     | Website:  https://openfoam.org
    \\  /    A nd           | Copyright (C) 2011-2018 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::coupling3d

Description

SourceFiles
    coupling3d.C

\*---------------------------------------------------------------------------*/

#ifndef coupling3d_H
#define coupling3d_H

#include "vector.H"
#include "scalar.H"
#include "List.H"

// NOTE:
// MUI header included if the switch -DUSE_MUI included during compilation.
#ifdef USE_MUI
    #include "mui.h"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class coupling3d Declaration
\*---------------------------------------------------------------------------*/

class coupling3d
{
    // Private data
private:
    struct interfaceDetails
    {
        word interfaceName;
        #ifdef USE_MUI
          mui::uniface<mui::config_3d>* mui_interface;
        #endif
        bool send;
        bool receive;
        bool smartSend;
    };

    word domainName_;
    List<coupling3d::interfaceDetails> interfaces_;
    List<word> interfaceNames_;
    List<bool> send_;
    List<bool> receive_;
    List<bool> smart_send_;

public:

    // Constructors

        //- Construct without explicit domain size
        coupling3d
        (
            word domainName,
            List<word>& interfaceNames,
            List<bool>& send,
            List<bool>& receive,
            List<bool>& smart_send
        );

    // Destructor

        ~coupling3d();

    // Member Functions
#ifdef USE_MUI
        mui::uniface<mui::config_3d>* getInterface(int index) const;
#endif
        size_t size() const;
        word getInterfaceName(int index) const;
        bool getInterfaceSendStatus(int index) const;
        bool getInterfaceReceiveStatus(int index) const;
        bool getInterfaceSmartSendStatus(int index) const;
        bool getInterfaceExtentsStatus(int index) const;
};

struct couplingInterface3d
{
    word domainName;
    scalar refLength;
    scalar refTime;
    coupling3d* interfaces;
};

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
