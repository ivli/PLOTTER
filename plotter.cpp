#include "plotter.h"
#include <thread>
#include <mutex>
#include <stdio.h>

std::ostream & operator << (std::ostream & os, const Motor& aMotor) {
	os << aMotor.iS << "; " << aMotor.iA << "; " << aMotor.iP << "; " << aMotor.iTP << "; " << aMotor.iV << std::endl;
	return os;
}

std::ostream & operator << (std::ostream & os, const Pen& aPen) {	
	nullptr != aPen.iX ? os << aPen.iX->getPosition() : os << "00"; 
	os << ";";
	nullptr != aPen.iY ? os << aPen.iY->getPosition() : os << "00";	
	return os;
}

class Logger {
public:
	static Period iTime ;
public:
	~Logger() {}
	Logger(const std::string& aName) { 
		iLog = std::shared_ptr<std::ofstream>(new std::ofstream(), [=](std::ofstream * os) {os->close();});
		iLog->open(aName + ".log"); 
	}
	std::ostream &stream() { return *iLog; };
	bool isOn() const { return iOn;}
	void setOn(bool aO) { iOn = aO; }

	void dump(const Pen& aPen) {
		if (isOn() && Pen::EToggle::ON == aPen.getToggle())
			*iLog << iTime << ";" << aPen << std::endl;
		else if (isOn() && Pen::EToggle::ON != aPen.getToggle()) {
			*iLog << iTime << "--;--" << std::endl;
			iOn = false;
		}
		else if (!isOn() && Pen::EToggle::ON == aPen.getToggle()) {
			*iLog << iTime << ";" << aPen << std::endl;
			iOn = true;
		}
		else {
		}		
	}
private:
	std::shared_ptr<std::ofstream> iLog;
	bool iOn = true;
};

Period Logger::iTime = .0;

void Pen::dump(Logger & aLog) {	
	aLog.dump(*this);
}

std::mutex  s_mtx;
std::thread s_sim;
std::thread s_logging;
bool running = true;

void Simulation::createMotor(const std::string& aName) {	
	if (iMotors.insert(std::pair<std::string, Motor>(aName, Motor())).second == false) {
		throw (std::out_of_range("a Motor with given name already exists: " + aName));
	}
}

void Simulation::createPen(const std::string& aName) {
	if (iPens.insert(std::pair<std::string, Pen>(aName, Pen())).second == false)
		throw (std::out_of_range("a Pen with given name already exists: " + aName));
	iLogs.insert(std::pair<std::string, Logger>(aName, Logger(aName)));
}

void Simulation::attach(const std::string& aPen, const std::string& aMotor, const EAxis& anAxis) {
	iPens.at(aPen).attach(iMotors.at(aMotor), anAxis);
}

void Simulation::toggle(const std::string& aPen, Pen::EToggle aToggle) {
	std::lock_guard<std::mutex> lck(s_mtx);
	iPens.at(aPen).setToggle(aToggle);
}

void Simulation::setMotorSmax(const std::string&  aMotor, const double anArg) {
	std::lock_guard<std::mutex> lck(s_mtx);
	iMotors.at(aMotor).setSpeed(anArg);
}

void Simulation::setMotorAupss(const std::string& aMotor, const double anArg) {
	std::lock_guard<std::mutex> lck(s_mtx);
	iMotors.at(aMotor).setAcceleration(anArg);
}

void Simulation::setMotorTP(const std::string&    aMotor, const double anArg) {
	std::lock_guard<std::mutex> lck(s_mtx);
	iMotors.at(aMotor).setTPosition(anArg);
}

void Simulation::start() {
	s_sim = std::thread([&]() {
		while (running) {
			iSimulationTime += iSimulationPeriod;
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(iSimulationPeriod * 1000)));
			doSim();
		}
	});

	s_logging = std::thread([&]() {
		while (running) {
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(iLoggingPeriod * 1000)));
			doLog();			
		} 
	});
}

void Simulation::stop() {	
	running = false;
}

void Simulation::setSimulationPeriod(const Period &aP) { iSimulationPeriod = aP; }
void Simulation::setLoggingPeriod(const Period &aP) { iLoggingPeriod = aP; }

void Simulation::doSim() {
	std::lock_guard<std::mutex> lck(s_mtx);
	
	const double tempi = iSimulationPeriod * iSimulationPeriod / 2.; // dT^2/2 - let's save some cycles

	for (auto& m : iMotors) {		
		const auto& pos_old = m.second.getPosition();
		const auto& acc_old = m.second.getAcceleration();
		const auto& vel_old = m.second.getVelocity();
		const auto& tp  = m.second.getTPosition();
        const double temp = acc_old * tempi;
		/*
		* a bit stright forward but shall work - while we are far from target it shall ride at full tilt and rein in a step away from TP 
		* DISCUSSION: well that's possible to estimate we aren't more than one step far from target and save some multiplications and additions
		* OTOH: foolowing calculation can get easily done using SIMD while this 'algo' would do it automatically with no additional effort 
		*/
		                         //A_aupss                              -A_aupss                                   A_aupss := 0
		const Velocity vel[] = { vel_old + acc_old* iSimulationPeriod,  vel_old - acc_old * iSimulationPeriod, vel_old               };
        const Position pos[] = {     pos_old + vel[0] + temp,               pos_old + vel[1] - temp,               pos_old + vel[2]  };
		const Position dp[]  = { std::abs(tp - pos[0]),                 std::abs(tp - pos[1]),                 std::abs(tp - pos[2]) };
	
		int ndx = 0;
		if (dp[1] < dp[0])
			ndx = 1;
		if (dp[2] < dp[ndx])
			ndx = 2;

		m.second.setVelocity(vel[ndx]);
		m.second.setPosition(pos[ndx]);

	}
}

void Simulation::doLog() {
	std::lock_guard<std::mutex> lck(s_mtx);
	
	Logger::iTime = iSimulationTime;

	for each (auto& p in iPens) {
		auto& l = iLogs.at(p.first);
		l.dump(p.second);
	}
}

void Simulation::dump(std::ostream& os) {
	os << "-----MOTORS-------" << std::endl;
	for each (auto& m in iMotors)
		os << m.first << ": " << m.second << std::endl;

	os << "-----PENS-------" << std::endl;
	for each (auto& p in iPens)
		os << p.first << ": " << p.second << std::endl;
}

int show_usage(int aCode) {	
	std::cout << "usage: \n\tplotter.exe  cfg_file\n";
	return aCode;
}

int main(int argc, char* argv[])
{
	Simulation sim;	
	Parser     cmd(sim);
	if (argc == 2) { //leave a backdoor to load config from a file 

		std::fstream file;
		file.open(argv[1]);

		if (file.is_open()) {
			cmd.parse_file(file);
			file.close();
		}
	}

	bool nem = true;
	
	do {
		std::string line;
		std::cout << ">";
		std::getline(std::cin, line);

		try {
			nem = cmd.parse_line(line);
		}
		
		catch (const std::out_of_range& ex) {
			std::cout << "error: " << ex.what() << std::endl;			
		}
		catch (const std::invalid_argument& ex) {
			std::cout << "error: " << ex.what() << std::endl;			
		}

		std::cout << (nem ? "OK" : "BYE BYE...") << std::endl;		
	} while (nem);

	sim.stop();

	s_sim.join();
	s_logging.join();
    return 0;
}

