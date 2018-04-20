#ifndef __PLOTTER_H___

#include <cctype>
#include <string>
#include <algorithm>
#include <map>
#include <iostream>
#include <fstream>
#include <chrono>
/*
* typedefs for types used
*/
typedef double Velocity;     //au per seconds
typedef double Speed;        //au per seconds
typedef double Acceleration; //au^2 per s 
typedef double Period;       //sec
typedef double Position;     //au

/*
* Coordinate system
*/
enum EAxis { X, Y };

/*
* Defaul values
*/
static const  Speed        KDefaultSpeed            = 1.0;
static const  Acceleration KDefaultAcceleration     = 1.0;
static const  Position     KDefaultTPosition        = 10.0;
static const  Position     KDefaultPosition         =  .0;
static const  Velocity     KDefaultVelocity         =  .0;
static const  Period       KDefaultSimulationPeriod = 0.1;
static const  Period       KDefaultLoggingPeriod    = 1.0;
/*
* implement Motor 
*/
class Motor {
public:
	void setSpeed(Speed aS) { iS = aS; } 
	const Speed& setSpeed() const { return iS; }
	void setAcceleration(Acceleration aA) { iA = aA; }
	const Acceleration& getAcceleration() const { return iA; }
	void setPosition(Position aP) { iP = aP; }
	const Position& getPosition() const { return iP; }
	void setTPosition(Position aP) { iTP = aP; }
	const Position& getTPosition() const { return iTP; }
	void setVelocity(Velocity aV) { iV = aV; }
	const Velocity& getVelocity() const { return iV; }

private:
	Speed        iS  = KDefaultSpeed;         //S_max_aups - maximal speed in abstract units per second(aups);
	Acceleration iA  = KDefaultAcceleration;  //A_aupss - acceleration absolute value in abstract units per seconds ^ 2 (aupss);
	Position     iTP = KDefaultTPosition;     //TP - target position in abstract units(au);
	Position     iP  = KDefaultPosition;      //P - current position in abstract units(au) relative to zero point;	
	Velocity     iV  = KDefaultVelocity;      //V - current velocity in abstract units per second(aups);
public: //misc
	friend  std::ostream & operator << (std::ostream & os, const Motor& aMotor);
};

class Logger;
/*
* Pen
*/
class Pen {
public:
	enum EToggle { OFF = 0, ON = 1};
public:	
	void  attach(const Motor& aM, const EAxis &anAxis) { anAxis == X ? iX = &aM : iY = &aM; }
	void  setToggle(EToggle aToggle) { iToggle = aToggle; }
	const EToggle getToggle() const { return iToggle; }
	void dump(Logger & aLog);
private:
	
private:
	EToggle iToggle = ON;
	const Motor *iX = nullptr;
	const Motor *iY = nullptr;
public: //misc
	friend std::ostream & operator << (std::ostream & os, const Pen& aPen);
};

/*
* context of the Simulation 
*/
class Simulation {
public:
	/**
	* creates new Motor
	* @param aName - a name of the new object
	* @throws std::out_of_range - in the case the name is already used 
	*/
	void createMotor(const std::string& aName);
	/**
	* creates new Pen
	* @param aName - a name of the new object
	* @throws std::out_of_range - in the case the name is already used
	*/
	void createPen(const std::string& aName);
	/**
	* attaches a Pen to a motor
	* @param aPen - a anme of the new object
	* @param aMotor - a anme of the new object
	* @param anAxis - an axis to attach
	* @throws std::out_of_range - in the case either pen or mothor of given name is not found 
	*/
	void attach(const std::string& aPen, const std::string& aMotor, const EAxis& anAxis);
	void toggle(const std::string& aPen, Pen::EToggle aToggle);	
	void setSimulationPeriod(const Period &aP);
	void setLoggingPeriod(const Period &aP);
	void setMotorSmax(const std::string&  aMotor, const double anArg);
	void setMotorAupss(const std::string& aMotor, const double anArg);
	void setMotorTP(const std::string&    aMotor, const double anArg);
	void start();
	void stop();
	void dump(std::ostream& os);
protected:
	void doSim();
	void doLog();

private:
	Period iSimulationPeriod = KDefaultSimulationPeriod;
	Period iLoggingPeriod    = KDefaultLoggingPeriod;
	Period iSimulationTime   = .0;
	std::map<std::string, Logger>  iLogs;
	std::map<std::string, Motor> iMotors;	
	std::map<std::string, Pen>   iPens;
	
};

/*
 * utility class implements statefull command processor 
 */
class Parser {
public:
	Parser(Simulation &aSim) :iSim(aSim) {}
	/**
	* implements parsing of an input stream containig a set of commands to be used thru CFG phase
	* @param anInputStream - an input stream
	* @return true if success otherwise false 
	*/
	bool parse_file(std::istream& anInputStream);
	/**
	* implements parsing of an input stream containig a command for SIM phase
	* @param anInputStream - an input stream
	* @return true if success otherwise false
	*/
	bool parse_line(std::string& aCommand);
protected:

	/**
	* implements parsing and command execution 
	* @param aCommand - a command to handle
	* @return true if command successed and execution shall be continued, false - if execution oughta get stopped
	* @throws invalid_argument
	* @throws out_of_range
    * 
	*/
	bool internal_parse_line(std::string& aCommand, Simulation& aSim);
private:
	bool iConfig = true;
	Simulation& iSim;
};

#endif

