//
// createCoupling.H
// ~~~~~~~~~~~~

    {
        //- If this is not a parallel run then need to first call MPI_Init for MUI (otherwise this is called during PStream creation)
        if (!args.parRunControl().parRun())
        {
            MPI_Init(&argc, &argv);
        }

        twoDInterfaces.interfaces = NULL;
        threeDInterfaces.interfaces = NULL;
        bool twoDCreated = false;
        bool threeDCreated = false;

        string inputFile("couplingDict");

        IOdictionary couplingDict
        (
            IOobject
            (
                inputFile,
                runTime.system(),
                mesh,
                IOobject::READ_IF_PRESENT,
                IOobject::NO_WRITE
            )
        );

        //Read coupling dictionary file if it exists
        if (couplingDict.headerOk())
        {
            word mainCouplingName;

            if (!(couplingDict.readIfPresent("couplingName", mainCouplingName)))
            {
                FatalIOErrorIn("", couplingDict)
                                << "Missing couplingName entry" << exit(FatalIOError);
            }

            twoDInterfaces.domainName = mainCouplingName;
            threeDInterfaces.domainName = mainCouplingName;

            scalar refLength;

			if (!(couplingDict.readIfPresent("refLength", refLength)))
			{
				FatalIOErrorIn("", couplingDict)
								<< "Missing refLength entry" << exit(FatalIOError);
			}

			twoDInterfaces.refLength = refLength;
			threeDInterfaces.refLength = refLength;

			scalar refTime;

			if (!(couplingDict.readIfPresent("refTime", refTime)))
			{
				FatalIOErrorIn("", couplingDict)
								<< "Missing refTime entry" << exit(FatalIOError);
			}

			twoDInterfaces.refTime = refTime;
			threeDInterfaces.refTime = refTime;

            const PtrList<entry> couplingConfigurationsList
            (
                couplingDict.lookup("couplingConfigurations")
            );

            //Iterate through configuration lists (2D/3D)
            forAll(couplingConfigurationsList, cC)
            {
                const entry& couplingConfigI = couplingConfigurationsList[cC];
                const dictionary& couplingConfigIDict = couplingConfigI.dict();

                wordList iFaceTOC(couplingConfigIDict.toc());

                List<word> interfaceNames(iFaceTOC.size());
                List<bool> send(iFaceTOC.size());
                List<bool> receive(iFaceTOC.size());
                List<bool> smart_send(iFaceTOC.size());

                //Iterate through interfaces
                forAll(iFaceTOC, cI)
                {
                    const dictionary& interfaceDict = couplingConfigIDict.subDict(iFaceTOC[cI]);

                    interfaceNames[cI] = static_cast<word>(interfaceDict.dictName());

                    if (interfaceDict.found("sending"))
                    {
                        send[cI] = Switch(interfaceDict.lookup("sending"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface sending entry" << exit(FatalIOError);
                    }

                    if (interfaceDict.found("receiving"))
                    {
                        receive[cI] = Switch(interfaceDict.lookup("receiving"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface receiving entry" << exit(FatalIOError);
                    }

                    if (interfaceDict.found("smart_send"))
                    {
                        smart_send[cI] = Switch(interfaceDict.lookup("smart_send"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface smart_send entry" << exit(FatalIOError);
                    }
                }

                if(iFaceTOC.size() > 0)
                {
                    if(couplingConfigIDict.dictName() == "TwoDInterfaces")
                    {
                       twoDInterfaces.interfaces = new coupling2d(mainCouplingName, interfaceNames, send, receive, smart_send);
                       twoDCreated = true;
                    }

                    if(couplingConfigIDict.dictName() == "ThreeDInterfaces")
                    {
                    	threeDInterfaces.interfaces = new coupling3d(mainCouplingName, interfaceNames, send, receive, smart_send);
						threeDCreated = true;
                    }
                }
            }
        }

        couplingDict.close();

        if(twoDCreated)
        {
        	//Announce send and receive spans for 2D interfaces that have smart send enabled
            for(size_t i=0; i<twoDInterfaces.interfaces->size(); i++)
            {
            	//Check if smart send should be enabled for this interface
            	if(twoDInterfaces.interfaces->getInterfaceSmartSendStatus(i))
				{
            		scalar oneOverRefLength = 1.0 / twoDInterfaces.refLength;

            		point meshMin(VGREAT, VGREAT, VGREAT);
					point meshMax(-VSMALL, -VSMALL, -VSMALL);

					const pointField& meshPoints = mesh.points();

					forAll(meshPoints, pts)
					{
						if(meshPoints[pts][0] < meshMin[0])
						{
							meshMin[0] = meshPoints[pts][0];
						}

						if(meshPoints[pts][1] < meshMin[1])
						{
							meshMin[1] = meshPoints[pts][1];
						}

						if(meshPoints[pts][2] < meshMin[2])
						{
							meshMin[2] = meshPoints[pts][2];
						}

						if(meshPoints[pts][0] > meshMax[0])
						{
							meshMax[0] = meshPoints[pts][0];
						}

						if(meshPoints[pts][1] > meshMax[1])
						{
							meshMax[1] = meshPoints[pts][1];
						}

						if(meshPoints[pts][2] > meshMax[2])
						{
							meshMax[2] = meshPoints[pts][2];
						}
					}

            		//Determine direction of 2D interface based on mesh extents (and check whether mesh is 3D)
					vector extents;
					extents[0] = meshMax[0] - meshMin[0];
					extents[1] = meshMax[1] - meshMin[1];
					extents[2] = meshMax[2] - meshMin[2];

					label extentsDir = -1; //0 = 3D; 1 = Y/Z; 2 = X/Z; 3 = X/Y

					if(extents[0] >= (0.0 - VSMALL) && extents[0] <= (0.0 + VSMALL)) // X extent is zero, 2D plane is in Y/Z
					{
						extentsDir = 1;
					}
					else if(extents[1] >= (0.0 - VSMALL) && extents[1] <= (0.0 + VSMALL)) // Y extent is zero, 2D plane is in X/Z
					{
						extentsDir = 2;
					}
					else if(extents[2] >= (0.0 - VSMALL) && extents[2] <= (0.0 + VSMALL)) // Z extent is zero, 2D plane is in X/Y
					{
						extentsDir = 3;
					}
					else
					{
						extentsDir = 0;
					}

					//Check mesh is 2D
					if(extentsDir > 0)
					{
						//Create a 2D box geometry
						mui::point2d start;
						mui::point2d end;

						if(extentsDir == 1) //Extents in Y/Z
						{
							start[0] = meshMin[1] * oneOverRefLength;
							start[1] = meshMin[2] * oneOverRefLength;

							end[0] = meshMax[1] * oneOverRefLength;
							end[1] = meshMax[2] * oneOverRefLength;
						}
						else if (extentsDir == 2) //Extents in X/Z
						{
							start[0] = meshMin[0] * oneOverRefLength;
							start[1] = meshMin[2] * oneOverRefLength;

							end[0] = meshMax[0] * oneOverRefLength;
							end[1] = meshMax[2] * oneOverRefLength;
						}
						else if (extentsDir == 3) //Extents in X/Y
						{
							start[0] = meshMin[0] * oneOverRefLength;
							start[1] = meshMin[1] * oneOverRefLength;

							end[0] = meshMax[0] * oneOverRefLength;
							end[1] = meshMax[1] * oneOverRefLength;
						}

						mui::geometry::box2d region_2d(start, end);

						//Interface is set to send
						if(twoDInterfaces.interfaces->getInterfaceSendStatus(i))
						{
							twoDInterfaces.interfaces->getInterface(i)->announce_send_span((runTime.startTime().value() / twoDInterfaces.refTime), runTime.endTime().value() / twoDInterfaces.refTime, region_2d);
						}

						//Interface is set to receive
						if(twoDInterfaces.interfaces->getInterfaceReceiveStatus(i))
						{
							twoDInterfaces.interfaces->getInterface(i)->announce_recv_span((runTime.startTime().value() / twoDInterfaces.refTime), runTime.endTime().value() / twoDInterfaces.refTime, region_2d);
						}

						mui::chrono_sampler_exact2d chrono_sampler(1e-9);

						twoDInterfaces.interfaces->getInterface(i)->commit(runTime.startTime().value() / twoDInterfaces.refTime);
						twoDInterfaces.interfaces->getInterface(i)->barrier(chrono_sampler.get_lower_bound(runTime.startTime().value() / twoDInterfaces.refTime));
						twoDInterfaces.interfaces->getInterface(i)->forget(chrono_sampler.get_upper_bound(runTime.startTime().value() / twoDInterfaces.refTime), true);
					}
					else if (extentsDir == 0) //Mesh is 3D
					{
						FatalIOErrorIn("", couplingDict)
									   << "Trying to create a 2D interface in a 3D mesh" << exit(FatalIOError);
					}
				}
            }
        }

        if(threeDCreated)
        {
        	//Announce send and receive spans for 3D interfaces that have domain extents to enable MUI smart send
			for(size_t i=0; i<threeDInterfaces.interfaces->size(); i++)
			{
				if(threeDInterfaces.interfaces->getInterfaceSmartSendStatus(i)) //Check if smart send should be enabled for this interface
				{
					scalar oneOverRefLength = 1.0 / threeDInterfaces.refLength;

					point meshMin(VGREAT, VGREAT, VGREAT);
					point meshMax(-VSMALL, -VSMALL, -VSMALL);

					const pointField& meshPoints = mesh.points();

					forAll(meshPoints, pts)
					{
						if(meshPoints[pts][0] < meshMin[0])
						{
							meshMin[0] = meshPoints[pts][0];
						}

						if(meshPoints[pts][1] < meshMin[1])
						{
							meshMin[1] = meshPoints[pts][1];
						}

						if(meshPoints[pts][2] < meshMin[2])
						{
							meshMin[2] = meshPoints[pts][2];
						}

						if(meshPoints[pts][0] > meshMax[0])
						{
							meshMax[0] = meshPoints[pts][0];
						}

						if(meshPoints[pts][1] > meshMax[1])
						{
							meshMax[1] = meshPoints[pts][1];
						}

						if(meshPoints[pts][2] > meshMax[2])
						{
							meshMax[2] = meshPoints[pts][2];
						}
					}

					//Check mesh is 3D
					vector extents;
					extents[0] = meshMax[0] - meshMin[0];
					extents[1] = meshMax[1] - meshMin[1];
					extents[2] = meshMax[2] - meshMin[2];

					bool isThreeD = true;

					//If one (or more) of the extents of the mesh is zero then mesh is not 3D
					if((extents[0] >= (0.0 - VSMALL) && extents[0]) <= (0.0 + VSMALL)
					   || (extents[1] >= (0.0 - VSMALL) && extents[1]) <= (0.0 + VSMALL)
					   || (extents[2] >= (0.0 - VSMALL) && extents[2]) <= (0.0 + VSMALL))
					{
						isThreeD = false;
					}

					if(isThreeD)
					{
						//Create a 3D box geometry based on mesh
						mui::point3d start(meshMin[0] * oneOverRefLength,
							 	           meshMin[1] * oneOverRefLength,
								           meshMin[2] * oneOverRefLength);
						mui::point3d end(meshMax[0] * oneOverRefLength,
								         meshMax[1] * oneOverRefLength,
								         meshMax[2] * oneOverRefLength);

						mui::geometry::box3d region_3d(start, end);

						//Interface is set to send
						if(threeDInterfaces.interfaces->getInterfaceSendStatus(i))
						{
							threeDInterfaces.interfaces->getInterface(i)->announce_send_span((runTime.startTime().value() / threeDInterfaces.refTime), (runTime.endTime().value() / threeDInterfaces.refTime), region_3d);
						}

						//Interface is set to receive
						if(threeDInterfaces.interfaces->getInterfaceReceiveStatus(i))
						{
							threeDInterfaces.interfaces->getInterface(i)->announce_recv_span((runTime.startTime().value() / threeDInterfaces.refTime), (runTime.endTime().value() / threeDInterfaces.refTime), region_3d);
						}

						mui::chrono_sampler_exact3d chrono_sampler(1e-9);

						threeDInterfaces.interfaces->getInterface(i)->commit(runTime.startTime().value() / threeDInterfaces.refTime);
						threeDInterfaces.interfaces->getInterface(i)->barrier(chrono_sampler.get_lower_bound(runTime.startTime().value() / threeDInterfaces.refTime));
						threeDInterfaces.interfaces->getInterface(i)->forget(chrono_sampler.get_upper_bound(runTime.startTime().value() / threeDInterfaces.refTime), true);
					}
					else
					{
						FatalIOErrorIn("", couplingDict)
									   << "Trying to create a 3D interface in a 2D mesh" << exit(FatalIOError);
					}
				}
			}
        }
    }
