#ifndef JOB_LAMBDA_H_INCLUDED
#define JOB_LAMBDA_H_INCLUDED

#include "pch.h"
#include "D3D12Helpers.h"
#include "D3D12Math.h"

class JobBase
{

public:

	JobBase() = default;
	~JobBase() = default;
	JobBase(std::wstring JobName)
		: JobName(std::move(JobName))
	{ }

	// Execute
	virtual void Execute(DXDevice * Device, DXGraphicsCommandList * CommandList) {}

	// Job Name
	std::wstring JobName;

private:
};

template<typename Lambda>
class JobLambda : public JobBase
{
public:

	// Verify that the amount of stuff captured by the pass lambda is reasonable.
	static constexpr UINT MaximumLambdaCaptureSize = 1024;
	//static_assert(sizeof(ExecuteLambdaType) <= kMaximumLambdaCaptureSize, "The amount of data of captured for the pass looks abnormally high.");

	JobLambda() = default;
	JobLambda(std::wstring JobName, Lambda&& Lambda)
		: JobBase(std::move(JobName))
		, Lambda(std::move(Lambda))
	{ }

	// Execute
	void Execute(DXDevice * Device, DXGraphicsCommandList * CommandList) override
	{
		DXLOG("Pass begin:				%ws \n", JobName.c_str());
		Lambda(Device, CommandList);
		DXLOG("Pass end:				%ws \n", JobName.c_str());
	}

private:

	Lambda Lambda;
};

#endif /* JOB_LAMBDA_H_INCLUDED */