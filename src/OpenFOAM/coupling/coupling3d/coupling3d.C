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

\*---------------------------------------------------------------------------*/

#include "coupling3d.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

//Construct without explicit domain sizes
Foam::coupling3d::coupling3d
(
    word domainName,
    List<word>& interfaceNames,
    List<bool>& send,
    List<bool>& receive,
    List<bool>& smart_send
)
:
    domainName_(domainName),
    interfaceNames_(interfaceNames),
    send_(send),
    receive_(receive),
    smart_send_(smart_send)
{
	interfaces_.setSize(interfaceNames_.size());

	forAll(interfaceNames_, i)
	{
		interfaceDetails newInterface;
		std::vector<std::string> interfaceList;

		newInterface.interfaceName = interfaceNames_[i];
		interfaceList.emplace_back(newInterface.interfaceName); //Need std::vector copy for MUI create_uniface function
		newInterface.send = send_[i];
		newInterface.receive = receive_[i];
		newInterface.smartSend = smart_send_[i];

		#ifdef USE_MUI
		  auto returnInterfaces_ = mui::create_uniface<mui::config_3d>(static_cast<std::string>(domainName_), interfaceList);
		  newInterface.mui_interface = returnInterfaces_[0].release();
		#endif

		interfaces_[i] = newInterface;
	}
}

// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::coupling3d::~coupling3d()
{
    #ifdef USE_MUI
        forAll(interfaces_, iface)
        {
            delete interfaces_[iface].mui_interface;
        }
    #endif
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

#ifdef USE_MUI
mui::uniface<mui::config_3d>* Foam::coupling3d::getInterface(int index) const
{
    return interfaces_[index].mui_interface;
}
#endif

size_t Foam::coupling3d::size() const
{
    return interfaces_.size();
}

Foam::word Foam::coupling3d::getInterfaceName(int index) const
{
    return interfaces_[index].interfaceName;
}

bool Foam::coupling3d::getInterfaceSendStatus(int index) const
{
    return interfaces_[index].send;
}

bool Foam::coupling3d::getInterfaceReceiveStatus(int index) const
{
    return interfaces_[index].receive;
}

bool Foam::coupling3d::getInterfaceSmartSendStatus(int index) const
{
    return interfaces_[index].smartSend;
}

// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //


// ************************************************************************* //
