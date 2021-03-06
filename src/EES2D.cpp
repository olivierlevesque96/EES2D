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

#include "io/InputParser.h"
#include "io/Su2Parser.h"
#include "mesh/Connectivity.h"
#include "mesh/Mesh.h"
#include "mesh/Metrics.h"
#include "utils/Timer.h"
#include "io/VtuWriter.h"
#include <iostream>
#include "solver/Simulation.h"
#include "solver/Solver.h"
#include "post/postProcess.h"
using ees2d::io::InputParser;
using ees2d::io::Su2Parser;
using ees2d::mesh::Connectivity;
using ees2d::mesh::Mesh;
using ees2d::mesh::MetricsData;
using ees2d::utils::Timer;
using ees2d::io::VtuWriter;
using ees2d::solver::Simulation;
using ees2d::solver::Solver;
using ees2d::post::PostProcess;
//using namespace ees2d::Utils;


int main(int argc, char *argv[]) {
	Timer Timeit("software runtime");
	std::cout << "Euler2D Software" << std::endl;

	std::string inputFilePath = argv[1];
  std::cout << inputFilePath << std::endl;
	InputParser simulationParameters{inputFilePath};
	simulationParameters.parse();


	Su2Parser parser(simulationParameters.m_meshFile);
	parser.Parse();

	Connectivity connectivity(parser);
	connectivity.solve();


	MetricsData metrics;
	metrics.compute(connectivity);



	Mesh mesh(connectivity, metrics);


  Simulation mysim(mesh,simulationParameters);


  Solver solver(mysim,mesh);
  solver.run();


  PostProcess mypost(mesh , mysim);
  mypost.solveCoefficients();

	if (simulationParameters.m_outputFormat == "VTK"){
		VtuWriter vtufile(simulationParameters.m_outputFile, connectivity, mesh,mysim);
		vtufile.writeSolution();
	}


	return 0;
}
