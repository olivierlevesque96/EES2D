/* ---------------------------------------------------------------------
 *
 * Copyright (C) 2020 - by the EES2D  authors
 *
 * This file is part of EES2D.
 *
 *   EES2D is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   EES2D is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with EES2D.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------
 *
 * Authors: Amin Ouled-Mohamed & Ali Omais, Polytechnique Montreal, 2020-
 */
#include "mesh/Connectivity.h"
#include "utils/Timer.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
using ees2d::io::Su2Parser;
using ees2d::mesh::Connectivity;


Connectivity::Connectivity(Su2Parser &parser) : m_parser(parser) {

	std::cout << " ---------------- Constructing connectivity tables !"
	             " -----------------"
	          << std::endl;


	// Initializing element surrounding points arrays
	for (auto &val : m_parser.get_NPSUE())
		m_esup1_size += val;
	m_esup1 = std::make_unique<uint32_t[]>(m_esup1_size);

	for (uint32_t i = 0; i < m_esup1_size; i++) {
		m_esup1[i] = 0;
	}

	m_esup2_size = m_parser.get_Ngrids() + 1;
	m_esup2 = std::make_unique<uint32_t[]>(m_esup2_size);

	for (uint32_t i = 0; i < m_esup2_size; i++) {
		m_esup2[i] = 0;
	}

	// Initializing points surrounding points arrays
	m_psup2_size = m_parser.get_Ngrids() + 1;
	m_lpoin_size = m_psup2_size - 1;
	m_psup2 = std::make_unique<uint32_t[]>(m_psup2_size);
	m_lpoin = new uint32_t[m_lpoin_size];// Temporary array to do calculations, not neeeded as retr
	m_psup1.reserve(m_lpoin_size);       //Ngrids == m_lpoin_size


	// Initializing Elements surrounding elements array
	m_esuel_size = m_parser.get_Nelems();
	m_elemToElem.reserve(m_esuel_size);

	// Initializing Node Surrounding Face arrays
	m_inpoe1 = new uint32_t[m_psup2_size];
}

//--------------------------------------------------------------------------

void ees2d::mesh::Connectivity::solve() {
	solveElemSurrNode();
	solveNodeSurrNode();
	solveElemSurrElem();
	solveNodeSurrFace();
	solveFaceSurrElem();
	solveElemSurrFace();
	delete[] m_inpoe1;
	m_inpoe1 = nullptr;
	delete[] m_lpoin;
	m_lpoin = nullptr;
}

//--------------------------------------------------------------------

const uint32_t &Connectivity::connecNodeSurrElement(const uint32_t &pointPos, const uint32_t &elementID) const {
	return m_parser.get_CONNEC()[m_parser.get_ElemIndex()[elementID] + pointPos];
}

//-----------------------------------------------------------------------

void Connectivity::solveElemSurrNode() {

	const uint32_t &Ngrids = m_parser.get_Ngrids();
	const uint32_t &Nelems = m_parser.get_Nelems();

	uint32_t ipoi1 = 0;
	uint32_t ipoin = 0;
	uint32_t istor = 0;

	// Pass 1 : Count the number of elements connected to each point
	for (uint32_t ielem = 0; ielem < Nelems; ielem++) {
		for (uint32_t inode = 0; inode < m_parser.get_NPSUE()[ielem]; inode++) {
			ipoi1 = connecNodeSurrElement(inode, ielem) + 1;
			m_esup2[ipoi1] = m_esup2[ipoi1] + 1;
		}
	}
	// Reshuffling pass 1
	for (uint32_t ipoin = 1; ipoin < Ngrids + 1; ipoin++) {
		m_esup2[ipoin] = m_esup2[ipoin] + m_esup2[ipoin - 1];
	}

	// Calculate sum of NPSUE


	//Pass 2 : Store the elements in m_esup1
	for (uint32_t ielem = 0; ielem < Nelems; ielem++) {
		for (uint32_t inode = 0; inode < m_parser.get_NPSUE()[ielem]; inode++) {
			ipoin = connecNodeSurrElement(inode, ielem);
			istor = m_esup2[ipoin] + 1;
			m_esup2[ipoin] = istor;
			m_esup1[istor - 1] = ielem;
		}
	}

	// Reshuffling pass 2
	for (uint32_t ipoin = Ngrids; ipoin > 0; ipoin--) {
		m_esup2[ipoin] = m_esup2[ipoin - 1];
	}
	m_esup2[0] = 0;

	std::cout << std::setw(40) << "Node to Elem connectivity : " << std::setw(6) << "Done\n";
}

//--------------------------------------------------------------

void Connectivity::solveNodeSurrNode() {

	for (uint32_t i = 0; i < m_lpoin_size; i++) {
		m_lpoin[i] = 0;
	}

	m_psup2[0] = 0;
	uint32_t istor = 0;
	uint32_t ielem = 0;
	uint32_t jpoin = 0;

	for (uint32_t ipoin = 0; ipoin < m_parser.get_Ngrids(); ipoin++) {

		for (uint32_t iesup = m_esup2[ipoin]; iesup < m_esup2[ipoin + 1]; iesup++) {
			ielem = m_esup1[iesup];

			for (uint32_t inode = 0; inode < m_parser.get_NPSUE()[ielem]; inode++) {
				jpoin = connecNodeSurrElement(inode, ielem);
				if ((jpoin != ipoin) && (m_lpoin[jpoin] != ipoin || ipoin == 0)) {
					istor += 1;
					m_psup1.push_back(jpoin);
					m_lpoin[jpoin] = ipoin;
				}
			}
		}
		m_psup2[ipoin + 1] = istor;
	}
	std::cout << std::setw(40) << "Node to node connectivity : " << std::setw(6) << "Done\n";
}

//------------------------------------------------------------------

void Connectivity::solveElemSurrElem() {

	uint32_t nnofa = 2;// Always two nodes per face (2D)
	std::array<uint32_t, 2> lhelp;
	uint32_t ipoin = 0;
	uint32_t jelem = 0;
	uint32_t nnofj = 2;
	uint32_t icoun = 0;
	uint32_t jpoin = 0;

	std::vector<std::vector<uint32_t>> m_lpofa3 = {{0, 1},
	                                               {1, 2},
	                                               {2, 0}};

	std::vector<std::vector<uint32_t>> m_lpofa4 = {{0, 1},
	                                               {1, 2},
	                                               {2, 3},
	                                               {3, 0}};

	std::vector<std::vector<uint32_t>> *m_lpofa;

	for (uint32_t i = 0; i < m_lpoin_size; i++) {
		m_lpoin[i] = 0;
	}

	for (uint32_t ielem = 0; ielem < m_parser.get_Nelems(); ielem++) {

		std::vector<uint32_t> temp_surr_elem = {};
		if (m_parser.get_NPSUE()[ielem] == 3) {
			m_lpofa = &m_lpofa3;
		} else if (m_parser.get_NPSUE()[ielem] == 4) {
			m_lpofa = &m_lpofa4;
		}

		for (uint32_t ifael = 0; ifael < m_parser.get_NPSUE()[ielem]; ifael++) {

			for (uint32_t nnofa = 0; nnofa < 2; nnofa++) {
				lhelp[nnofa] = connecNodeSurrElement((*m_lpofa)[ifael][nnofa], ielem);
				m_lpoin[lhelp[nnofa]] = 1;
			}
			ipoin = lhelp[0];

			for (uint32_t istor = m_esup2[ipoin]; istor < m_esup2[ipoin + 1]; istor++) {
				jelem = m_esup1[istor];

				if (jelem != ielem) {
					for (uint32_t jfael = 0; jfael < m_parser.get_NPSUE()[ielem]; jfael++) {
						if (nnofj == nnofa) {
							icoun = 0;
							for (uint32_t jnofa = 0; jnofa < nnofa; jnofa++) {
								jpoin = connecNodeSurrElement((*m_lpofa)[jfael][jnofa], jelem);
								icoun = icoun + m_lpoin[jpoin];
							}
							if (icoun == nnofa) {
								temp_surr_elem.push_back(jelem);
							}
						}
					}
				}
			}
			for (uint32_t nnofa = 0; nnofa < 2; nnofa++) {
				m_lpoin[lhelp[nnofa]] = 0;
			}
		}
		m_elemToElem.push_back(temp_surr_elem);
	}
	std::cout << std::setw(40) << "Element to element connectivity : " << std::setw(6) << "Done\n";
}

//---------------------------------------------------------------------

void Connectivity::solveNodeSurrFace() {

	for (uint32_t i = 0; i < m_lpoin_size; i++) {
		m_lpoin[i] = 0;
	}

	m_inpoe1[0] = 0;
	uint32_t nedge = 0;
	uint32_t ielem = 0;
	uint32_t jpoin = 0;

	std::vector<uint32_t> temp = {};
	std::vector<uint32_t> doubles = {};
	std::vector<uint32_t> temp2 = {};
	uint32_t nodeToPush1;
	uint32_t nodeToPush2;
	if (m_parser.get_NPSUE()[0] == 4) {
		uint32_t node1;
		uint32_t node2;
		uint32_t node3;
		uint32_t node4;
		for (uint32_t ielem = 0; ielem < m_parser.get_Nelems(); ielem++) {
			temp = {};

			node1 = connecNodeSurrElement(0, ielem);
			node2 = connecNodeSurrElement(1, ielem);
			node3 = connecNodeSurrElement(2, ielem);
			node4 = connecNodeSurrElement(3, ielem);
			temp.push_back(node1);
			temp.push_back(node2);
			temp.push_back(node3);
			temp.push_back(node4);
			for (uint32_t i = 0; i < 4; i++) {
				temp2 = {};
				nodeToPush1 = temp[i % 4];// 4%4 = 0 go back to node 0
				nodeToPush2 = temp[(i + 1) % 4];
				if (nodeToPush1 > nodeToPush2) {
					std::swap(nodeToPush1, nodeToPush2);
				}
				temp2.push_back(nodeToPush1);
				temp2.push_back(nodeToPush2);
				m_faceToNode.push_back(temp2);
			}
		}
		std::sort(m_faceToNode.begin(), m_faceToNode.end());
		auto last = std::unique(m_faceToNode.begin(), m_faceToNode.end());
		m_faceToNode.erase(last, m_faceToNode.end());

	} else if (m_parser.get_NPSUE()[0] == 3) {
		bool skip = false;

		for (uint32_t ipoin = 0; ipoin < m_parser.get_Ngrids(); ipoin++) {

			for (uint32_t iesup = m_esup2[ipoin]; iesup < m_esup2[ipoin + 1]; iesup++) {
				ielem = m_esup1[iesup];

				for (uint32_t inode = 0; inode < m_parser.get_NPSUE()[ielem]; inode++) {
					jpoin = connecNodeSurrElement(inode, ielem);
					if ((jpoin != ipoin) && (m_lpoin[jpoin] != ipoin || ipoin == 0)) {
						if (ipoin < jpoin) {
							if (ipoin == 0) {
								skip = false;
								for (auto &value : doubles) {
									if (jpoin == value) {
										skip = true;
										break;
									}
								}
								if (skip == true) {

								} else if (skip == false) {
									doubles.push_back(jpoin);
									nedge += 1;
									temp = {};
									temp.push_back(ipoin);
									temp.push_back(jpoin);
									m_faceToNode.push_back(temp);
									m_lpoin[jpoin] = ipoin;
								}

							} else {// Not sure in which order faces should be created
								nedge += 1;
								temp = {};
								temp.push_back(ipoin);
								temp.push_back(jpoin);
								m_faceToNode.push_back(temp);
								m_lpoin[jpoin] = ipoin;
							}
						}
					}
				}
			}
			m_inpoe1[ipoin + 1] = nedge;
		}
	}
	std::cout << std::setw(40) << "Face to node connectivity : " << std::setw(6) << " Done\n";
}

//----------------------------------------------------------------------

void Connectivity::solveFaceSurrElem() {

	uint32_t ipoi1 = 0;
	uint32_t ipoi2 = 0;
	uint32_t ipmin = 0;
	uint32_t ipmax = 0;

	if (m_parser.get_NPSUE()[0] == 4) {
		// FaceSurrElem is not used for a face based solver

	} else if (m_parser.get_NPSUE()[0] == 3) {
		std::vector<std::vector<uint32_t>> m_lpoed3 = {{0, 1},
		                                               {1, 2},
		                                               {2, 0}};

		std::vector<std::vector<uint32_t>> m_lpoed4 = {{0, 1},
		                                               {1, 2},
		                                               {2, 3},
		                                               {3, 0}};

		std::vector<std::vector<uint32_t>> *m_lpoed;

		m_elemToFace.reserve(m_parser.get_Nelems());

		std::vector<uint32_t> temp = {};

		for (uint32_t ielem = 0; ielem < m_parser.get_Nelems(); ielem++) {
			temp = {};
			for (uint32_t iedel = 0; iedel < m_parser.get_NPSUE()[ielem]; iedel++) {

				if (m_parser.get_NPSUE()[ielem] == 3) {
					m_lpoed = &m_lpoed3;
				} else if (m_parser.get_NPSUE()[ielem] == 4) {
					m_lpoed = &m_lpoed4;
				}

				ipoi1 = connecNodeSurrElement((*m_lpoed)[iedel][0], ielem);
				ipoi2 = connecNodeSurrElement((*m_lpoed)[iedel][1], ielem);
				ipmin = std::min(ipoi1, ipoi2);
				ipmax = std::max(ipoi1, ipoi2);

				for (uint32_t iedge = m_inpoe1[ipmin]; iedge < m_inpoe1[ipmin + 1]; iedge++) {
					if (m_faceToNode[iedge][1] == ipmax) {
						temp.push_back(iedge);
					}
				}
			}
			m_elemToFace.push_back(temp);
		}
	}
	std::cout << std::setw(40) << "Element to Face connectivity : " << std::setw(6) << " Done\n";
}

//-------------------------------------------------------------------------------------------------

void Connectivity::solveElemSurrFace() {

	m_faceToElem.reserve(m_faceToNode.size());


	std::vector<uint32_t> temp;
	temp.reserve(2);
	const uint32_t &lastBCVectorLength = m_parser.get_boundaryConditions().back().size();


	for (uint32_t nface = 0; nface < m_faceToNode.size(); nface++) {
		temp = {};
		const uint32_t &node1 = m_faceToNode[nface][0];
		const uint32_t &node2 = m_faceToNode[nface][1];
		for (uint32_t elemsIndexNode1 = m_esup2[node1]; elemsIndexNode1 < m_esup2[node1 + 1]; elemsIndexNode1++) {
			const uint32_t &ElemNode1 = m_esup1[elemsIndexNode1];
			for (uint32_t elemsIndexNode2 = m_esup2[node2]; elemsIndexNode2 < m_esup2[node2 + 1]; elemsIndexNode2++) {
				const uint32_t &ElemNode2 = m_esup1[elemsIndexNode2];
				if (ElemNode1 == ElemNode2) {
					temp.push_back(ElemNode1);
					break;
				}
			}
		}
		// Add Boundary element with the corresponding boundary ID
		if (temp.size() == 1) {
			for (uint32_t i = 0; i < lastBCVectorLength; i++) {
				if (node1 == m_parser.get_boundaryConditions().back()[i]) {
					if (node2 == m_parser.get_boundaryConditions()[i][1]) {
						temp.push_back(m_parser.get_boundaryConditions()[i][2]);
						m_elemToElem[temp[0]].push_back(m_parser.get_boundaryConditions()[i][2]);
						break;
					}
				}
			}
		}
		m_faceToElem.push_back(temp);
	}

	std::cout << std::setw(40) << "Face to Element connectivity : " << std::setw(6) << " Done\n";
}
