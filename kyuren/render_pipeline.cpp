#include "data.h"

#include "core/type.h"
#include "core/math.h"
#include "core/dev_util.h"

#include "gpu/gpu.h"
#include "runtime/system.h"

#include "memory/allocators/scope_allocator.h"

#include <cstring>

namespace KyuRen {
	
	uint64 RenderJob::ParamList::hashName(const char* name) {
		return hashFNV1((uint8*) name, strlen(name));
	}

	RenderJobID RenderPipeline::addJob(RenderJob* renderJob) {
		uint32 jobID = _jobs.add({ renderJob, Soul::Array<Socket>(), Soul::Array<OutputEdge>() });
		uint32 inputCount = renderJob->getInputCount();
		uint32 inputOutputCount = renderJob->getInputOutputCount();
		_jobs[jobID].inputs.resize(inputCount + inputOutputCount);
		for (Socket& input : _jobs[jobID].inputs) {
			input.jobID = RENDER_JOB_ID_NULL;
		}
		return RenderJobID(jobID);
	}

	void RenderPipeline::_removeSourceLink(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetJob, uint32 targetParamIndex) {
		RenderJobInstance& srcJobInstance = _jobs[sourceJob.id];
		for (OutputEdge& outputEdge : srcJobInstance.outputs) {
		
			if (outputEdge.sourceParamIndex == sourceParamIndex && outputEdge.targetSocket == Socket(targetJob, targetParamIndex)) {
				outputEdge = srcJobInstance.outputs.back();
				srcJobInstance.outputs.pop();
				break;
			}
		}
	}

	void RenderPipeline::_removeTargetLink(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetJob, uint32 targetParamIndex) {
		RenderJobInstance& targetJobInstance = _jobs[targetJob.id];
		SOUL_ASSERT(0, targetJobInstance.inputs[targetParamIndex] == Socket(sourceJob, sourceParamIndex), "");
		targetJobInstance.inputs[targetParamIndex] = Socket(RENDER_JOB_ID_NULL, 0);
	}

	void RenderPipeline::_removeEdge(RenderJobID sourceJob, uint32 sourceParamIndex, RenderJobID targetJob, uint32 targetParamIndex) {
		RenderJobInstance& srcJobInstance = _jobs[sourceJob.id];
		RenderJobInstance& dstJobInstance = _jobs[targetJob.id];

		_removeSourceLink(sourceJob, sourceParamIndex, targetJob, targetParamIndex);
		_removeTargetLink(sourceJob, sourceParamIndex, targetJob, targetParamIndex);

	}

	void RenderPipeline::removeJob(RenderJobID jobID) {
		for (int i = 0; i < _jobs[jobID.id].inputs.size(); i++) {
			Socket& input = _jobs[jobID.id].inputs[i];
			_removeSourceLink(input.jobID, input.paramIndex, jobID, i);
		}
		for (OutputEdge& outputEdge : _jobs[jobID.id].outputs) {
			_removeTargetLink(jobID, outputEdge.sourceParamIndex, outputEdge.targetSocket.jobID, outputEdge.targetSocket.paramIndex);
		}
		_jobs.remove(jobID.id);
	}

	void RenderPipeline::connect(RenderJobID sourceJobID, const char* sourceParam, RenderJobID targetJobID, const char* targetParam) {
		RenderJob* srcRenderJob = _jobs[sourceJobID.id].renderJob;
		RenderJob* dstRenderJob = _jobs[targetJobID.id].renderJob;
		int32 srcParamIndex = srcRenderJob->getOutputIndex(sourceParam);
		int32 targetParamIndex = dstRenderJob->getInputIndex(targetParam);
		_jobs[sourceJobID.id].outputs.add({ uint32(srcParamIndex), { targetJobID, uint32(targetParamIndex) } });
		_jobs[targetJobID.id].inputs[targetParamIndex] = { sourceJobID, uint32(srcParamIndex) };
	}

	void RenderPipeline::setOutput(RenderJobID jobID, const char* param) {
		RenderJob* renderJob = _jobs[jobID.id].renderJob;
		int32 paramIndex = renderJob->getOutputIndex(param);
		_output = { jobID, uint32(paramIndex) };
	}

	void RenderPipeline::compile() {
		
		_executionOrder.resize(0);

		Soul::Array<uint32> inputCount(Soul::Runtime::GetTempAllocator());
		inputCount.resize(_jobs.size());
		
		for (int i = 0; i < _jobs.size(); i++) {
			RenderJobInstance& instance = _jobs[i];
			uint32 count = 0;
			for (Socket& input : instance.inputs) {
				if (input.jobID != RENDER_JOB_ID_NULL) {
					count++;
				}
			}
			inputCount[i] = count;
		}

		Soul::Array<uint32> topoSortQueue(Soul::Runtime::GetTempAllocator());
		for (int i = 0; i < inputCount.size(); i++) {
			if (inputCount[i] == 0) {
				topoSortQueue.add(i);
			}
		}

		while (topoSortQueue.size() != 0) {
			uint32 idx = topoSortQueue.back();
			topoSortQueue.pop();

			RenderJobID jobID = RenderJobID(_jobs.getPackedID(idx));
			_executionOrder.add(jobID);

			RenderJobInstance& instance = _jobs[jobID.id];
			
			for (OutputEdge& output : instance.outputs) {
				uint32 outputJobIndex = _jobs.getInternalID(output.targetSocket.jobID.id);
				inputCount[outputJobIndex]--;
				if (inputCount[outputJobIndex] == 0) {
					topoSortQueue.add(outputJobIndex);
				}
			}
		}
		
	}

	void RenderPipeline::execute(char* pixels) {

		Soul::Memory::ScopeAllocator<> executeAllocator("Render Pipeline Execute Allocator");
		Soul::Array<RenderJobOutputs*> outputsList(&executeAllocator);
		outputsList.reserve(_jobs.size());

		for (RenderJobInstance& job : _jobs) {
			RenderJobOutputs* outputs = executeAllocator.create<RenderJobOutputs>(job.renderJob, &executeAllocator);
			outputsList.add(outputs);
		}

		Soul::GPU::RenderGraph renderGraph;
		for (RenderJobID jobID : _executionOrder) {
			RenderJobInstance& jobInstance = _jobs[jobID.id];
			RenderJobInputs inputs(jobInstance.renderJob, &executeAllocator);
			for (int i = 0; i < jobInstance.inputs.size(); i++) {
				RenderJobID sourceJobID = jobInstance.inputs[i].jobID;
				SOUL_LOG_INFO("Source job id %d", sourceJobID.id);
				RenderJobOutputs* sourceJobOutputs = outputsList[_jobs.getInternalID(sourceJobID.id)];
				inputs[i] = (*sourceJobOutputs)[jobInstance.inputs[i].paramIndex]; 
			}
			SOUL_LOG_INFO("Execution 4 %d", jobID.id);
			RenderJobOutputs& outputs = *outputsList[_jobs.getInternalID(jobID.id)];
			SOUL_LOG_INFO("Execution 5 %d", jobID.id);
			jobInstance.renderJob->execute(&renderGraph, inputs, outputs);

			if (_output.jobID == jobID) {
				SOUL_LOG_INFO("Output texture node id = %d", outputs[_output.paramIndex].val.textureNodeID.id);
				renderGraph.exportTexture(outputs[_output.paramIndex].val.textureNodeID, pixels);
			}
		}
		_gpuSystem->renderGraphExecute(renderGraph);
		_gpuSystem->frameFlush();
		renderGraph.cleanup();
	}

}
