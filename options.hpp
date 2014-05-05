

#ifndef OPTIONS_HPP_GY8LUN
#define OPTIONS_HPP_GY8LUN

#include <string>

struct Options {
	std::string inputBenchmark, outputFile;

	bool useNetDecomposition = true;
	bool useNetOrdering = true;
	bool emitSVG = false;
	
	enum CostFunction {
		Standard, NC
	};
	
	CostFunction costFunction = Standard;
	
	void setCostFunction(const std::string &s)
	{
		if(s.empty() || s == "standard")
		{
			costFunction = Standard;
		}
		else if(s == "nc")
		{
			costFunction = NC;
		}
		else
		{
			throw std::runtime_error("Unknwon cost function " + s + ". Options are 'standard', 'nc'.");
		}
	}
};


#endif // OPTIONS_HPP_GY8LUN