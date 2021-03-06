/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <cstdio>
#include <signal.h>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;
namespace po = boost::program_options;

#include <dynamo/simulation/simulation.hpp>
#include <dynamo/dynamics/BC/include.hpp>
#include <dynamo/dynamics/dynamics.hpp>
#include <dynamo/dynamics/systems/ghost.hpp>
#include <dynamo/schedulers/include.hpp>
#include <dynamo/inputplugins/include.hpp>
#include <magnet/exception.hpp>
dynamo::Simulation sim;

int
main(int argc, char *argv[])
{
  //The following macro converts the GITHASH define to a C style
  //string, the boost build system won't let us define strings on the
  //command line.
#define VALUE_TO_STRING(val) #val
#define STR(val) VALUE_TO_STRING(val)
  std::cout << "dynamod  Copyright (C) 2011  Marcus N Campbell Bannerman\n"
	    << "This program comes with ABSOLUTELY NO WARRANTY.\n"
	    << "This is free software, and you are welcome to redistribute it\n"
	    << "under certain conditions. See the licence you obtained with\n"
	    << "the code\n"
	       "Git Checkout Hash " << STR(GITHASH) << "\n\n";
  ////////////////////////PROGRAM OPTIONS!!!!!!!!!!!!!!!!!!!!!!!
  try 
    {
      po::options_description allopts("General Options"), loadopts("Load Config File Options"),
	hiddenopts("Packing Mode Options (description of each for each mode is given by --packer-mode-help)"),
	helpOpts;

      allopts.add_options()
	("help,h", "Produces this message OR if --packer-mode/-m is set, it lists the specific options available for that packer mode.")
	("out-config-file,o", 
	 po::value<string>()->default_value("config.out.xml.bz2"), 
	 "Configuration output file.")
	("random-seed,s", po::value<unsigned int>(),
	 "Seed value for the random number generator.")
	("rescale-T,r", po::value<double>(), 
	 "Rescales the kinetic temperature of the input/generated config to this value.")
	("thermostat,T", po::value<double>(),
	 "Change the thermostat temperature (will add a thermostat and set the Ensemble to NVT if needed).")
	("zero-momentum,Z", "Zeros the total momentum of the input/generated config.")
	("zero-com", "Zeros the centre of mass of the input/generated config.")
	("zero-vel", po::value<size_t>(), "Sets the velocity in the [arg=0,1,or 2] dimension of each particle to zero.")
	("set-com-vel", po::value<std::string>(), 
	 "Sets the velocity of the COM of the system (format x,y,z no spaces).")
	("mirror-system,M",po::value<unsigned int>(), 
	 "Mirrors the particle co-ordinates and velocities. Argument is "
	 "dimension to reverse/mirror.")
	("round", "Output the XML config file with one less digit of accuracy to remove"
	 "rounding errors (used in the test harness).")
	("unwrapped", "Don't apply the boundary conditions of the system when writing out the particle positions.")
	("check", "Runs tests on the configuration to ensure the system is not in an invalid state.")
	;

      loadopts.add_options()
	("config-file", po::value<string>(), 
	 "Config file to initialise from (Non packer mode).")
	;
      
      
      helpOpts.add(allopts);
      helpOpts.add(dynamo::IPPacker::getOptions());
             
      allopts.add(loadopts);
      allopts.add(dynamo::IPPacker::getOptions());
      
      hiddenopts.add_options()
	("b1", "boolean option one.")
	("b2", "boolean option two.")
	("i1", po::value<size_t>(), "integer option one.")
	("i2", po::value<size_t>(), "integer option two.")
	("s1", po::value<std::string>(), "string option one.")
	("s2", po::value<std::string>(), "string option two.")
	("f1", po::value<double>(), "double option one.")
	("f2", po::value<double>(), "double option two.")
	("f3", po::value<double>(), "double option three.")
	("f4", po::value<double>(), "double option four.")
	("f5", po::value<double>(), "double option five.")
	("f6", po::value<double>(), "double option six.")
	("f7", po::value<double>(), "double option seven.")
	;

      allopts.add(hiddenopts);

      po::positional_options_description p;
      p.add("config-file", 1);
      
      po::variables_map vm;
      po::store(po::command_line_parser(argc, argv).
		options(allopts).positional(p).run(), vm);
      po::notify(vm);
      
      if ((!vm.count("packer-mode")
	   && (vm.count("help") || !vm.count("config-file"))))
	{
	  cout << "Usage : dynamod <OPTIONS>...[CONFIG FILE]\n"
	       << " Either modifies a config file (if a file name is passed as an argument) OR generates a new config file depending on the packing mode (if --packer-mode/-m is used).\n" 
	       << helpOpts;
	  return 1;
	}

      if (vm.count("random-seed"))
	sim.setRandSeed(vm["random-seed"].as<unsigned int>());
      
      ////////////////////////Simulation Initialisation!!!!!!!!!!!!!
      //Now load the config
      if (!vm.count("config-file"))
	{
	  dynamo::IPPacker plug(vm, &sim);
	  plug.initialise();

	  //We don't zero momentum and rescale for certain packer modes
	  if ((vm["packer-mode"].as<size_t>() != 23)
	      && (vm["packer-mode"].as<size_t>() != 25))
	    {
	      dynamo::InputPlugin(&sim, "Rescaler").zeroMomentum();
	      dynamo::InputPlugin(&sim, "Rescaler").rescaleVels(1.0);
	    }
	  sim.configLoaded();
	}
      else
	sim.loadXMLfile(vm["config-file"].as<string>());
  
      sim.setTrajectoryLength(0);

      if (vm.count("thermostat"))
	{
	  dynamo::System* thermostat = sim.getSystem("Thermostat");
	  if (thermostat == NULL)
	    {
	      sim.addSystem(dynamo::shared_ptr<dynamo::System>
			    (new dynamo::SysAndersen(&sim, 1.0, 1.0, "Thermostat")));
	      thermostat = sim.getSystem("Thermostat");
	    }

	  if (dynamic_cast<const dynamo::SysAndersen*>(thermostat) == NULL)
	    M_throw() << "Could not upcast thermostat to Andersens";
	  
	  static_cast<dynamo::SysAndersen*>(thermostat)->setReducedTemperature(vm["thermostat"].as<double>());
	  
	  //Install a NVT Ensemble
	  sim.getEnsemble().reset(new dynamo::EnsembleNVT(&sim));
	}

      sim.initialise();      
      
      //Here we modify the sim accordingly      

      if (vm.count("zero-momentum"))
	dynamo::InputPlugin(&sim, "MomentumZeroer")
	  .zeroMomentum();	

      if (vm.count("check"))
	sim.checkSystem();

      if (vm.count("zero-com"))
	dynamo::InputPlugin(&sim, "CentreOfMassZeroer")
	  .zeroCentreOfMass();	

      if (vm.count("rescale-T"))
	dynamo::InputPlugin(&sim, "Rescaler")
	  .rescaleVels(vm["rescale-T"].as<double>());

      if (vm.count("mirror-system"))
	dynamo::InputPlugin(&sim, "Mirrorer").
	  mirrorDirection(vm["mirror-system"].as<unsigned int>());

      if (vm.count("set-com-vel"))
	{
	  boost::tokenizer<boost::char_separator<char> > 
	    tokens(vm["set-com-vel"].as<std::string>(), boost::char_separator<char>(","));
	  
	  boost::tokenizer<boost::char_separator<char> >::iterator details_iter = tokens.begin();

	  dynamo::Vector vel(0,0,0);

	  if (details_iter == tokens.end()) M_throw() << "set-com-vel requires 3 components";
	  vel[0] = boost::lexical_cast<double>(*(details_iter++));
	  if (details_iter == tokens.end()) M_throw() << "set-com-vel requires 3 components";	  
	  vel[1] = boost::lexical_cast<double>(*(details_iter++));
	  if (details_iter == tokens.end()) M_throw() << "set-com-vel requires 3 components";
	  vel[2] = boost::lexical_cast<double>(*(details_iter));
	  
	  dynamo::InputPlugin(&sim, "velSetter")
	    .setCOMVelocity(vel);
	}

      if (vm.count("zero-vel"))
	dynamo::InputPlugin(&sim, "Vel-Component-Zeroer")
	  .zeroVelComp(vm["zero-vel"].as<size_t>());


      //Write out now we've changed the system
      sim.getHistory() << "configmod run as so\n";
      for (int i = 0; i< argc; i++)
	sim.getHistory() << argv[i] << " ";
      sim.getHistory() << "\nGIT hash " << STR(GITHASH);
      sim.writeXMLfile(vm["out-config-file"].as<string>(), 
		       !vm.count("unwrapped"), vm.count("round"));
    }
  catch (std::exception &cep)
    {
      std::cout << "\nReached Main Error Loop"
		<< "\nOutputting results so far and shutting down"
		<< "\nBad configuration written to config.error.xml"
		<< cep.what();
      
      try
	{ sim.writeXMLfile("config.error.xml.bz2"); }
      catch (std::exception &cep)
	{
	  std::cout << "\nFailed to output error config"
		    << cep.what() << "\n";
	}

      std::cout << "\n";

      return 1;
    }

  return 0;
}
