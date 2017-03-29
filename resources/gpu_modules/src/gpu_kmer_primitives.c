/*
 *  GEM-Cutter "Highly optimized genomic resources for GPUs"
 *  Copyright (c) 2013-2016 by Alejandro Chacon    <alejandro.chacond@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *  Some rights reserved. See LICENSE, AUTHORS.
 *  @license GPL-3.0+ <http://www.gnu.org/licenses/gpl-3.0.en.html>
 */

#ifndef GPU_KMER_PRIMITIVES_C_
#define GPU_KMER_PRIMITIVES_C_

#include "../include/gpu_kmer_primitives.h"

/************************************************************
Functions to get the K-MER filter buffers
************************************************************/

uint32_t gpu_kmer_buffer_get_max_qry_bases_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.maxBases);
}

uint32_t gpu_kmer_buffer_get_max_candidates_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.maxCandidates);
}

uint32_t gpu_kmer_buffer_get_max_queries_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.maxQueries);
}

gpu_kmer_qry_entry_t* gpu_kmer_buffer_get_queries_(const void* const kmerBuffer){
  const gpu_buffer_t * const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.queries.h_queries);
}

gpu_kmer_cand_info_t* gpu_kmer_buffer_get_candidates_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.candidates.h_candidates);
}

gpu_kmer_qry_info_t* gpu_kmer_buffer_get_qry_info_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.queries.h_queryInfo);
}

gpu_kmer_alg_entry_t* gpu_kmer_buffer_get_alignments_(const void* const kmerBuffer){
  const gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  return(mBuff->data.kmer.alignments.h_alignments);
}


/************************************************************
Functions to init all the BPM resources
************************************************************/

float gpu_kmer_size_per_candidate(const uint32_t averageQuerySize, const uint32_t candidatesPerQuery)
{
  const size_t bytesPerQuery        = averageQuerySize * sizeof(gpu_kmer_qry_entry_t) + sizeof(gpu_kmer_qry_info_t);
  const size_t bytesCandidate       = sizeof(gpu_kmer_cand_info_t);
  const size_t bytesResult          = sizeof(gpu_kmer_alg_entry_t);
  // Calculate the necessary bytes for each K-MER filter operation
  return((bytesPerQuery/(float)candidatesPerQuery) + bytesCandidate + bytesResult);
}

void gpu_kmer_reallocate_host_buffer_layout(gpu_buffer_t* mBuff)
{
  void* rawAlloc = mBuff->h_rawData;
  //Adjust the host buffer layout (input)
  mBuff->data.kmer.queries.h_queries = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.queries.h_queries + mBuff->data.kmer.maxBases);
  mBuff->data.kmer.queries.h_queryInfo = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.queries.h_queryInfo + mBuff->data.kmer.maxQueries);
  mBuff->data.kmer.candidates.h_candidates = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.candidates.h_candidates + mBuff->data.kmer.maxCandidates);
  //Adjust the host buffer layout (output)
  mBuff->data.kmer.alignments.h_alignments = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.alignments.h_alignments + mBuff->data.kmer.maxAlignments);
}

void gpu_kmer_reallocate_device_buffer_layout(gpu_buffer_t* mBuff)
{
  void* rawAlloc = mBuff->d_rawData;
  //Adjust the host buffer layout (input)
  mBuff->data.kmer.queries.d_queries = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.queries.d_queries + mBuff->data.kmer.maxBases);
  mBuff->data.kmer.queries.d_queryInfo = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.queries.d_queryInfo + mBuff->data.kmer.maxQueries);
  mBuff->data.kmer.candidates.d_candidates = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.candidates.d_candidates + mBuff->data.kmer.maxCandidates);
  //Adjust the host buffer layout (output)
  mBuff->data.kmer.alignments.d_alignments = GPU_ALIGN_TO(rawAlloc,16);
  rawAlloc = (void *) (mBuff->data.kmer.alignments.d_alignments + mBuff->data.kmer.maxAlignments);
}

void gpu_kmer_init_buffer_(void* const kmerBuffer, const uint32_t averageQuerySize, const uint32_t candidatesPerQuery)
{
  gpu_buffer_t* const mBuff                   = (gpu_buffer_t *) kmerBuffer;
  const double        sizeBuff                = mBuff->sizeBuffer * 0.95;
  const uint32_t      numInputs               = (uint32_t)(sizeBuff / gpu_kmer_size_per_candidate(averageQuerySize, candidatesPerQuery));
  const uint32_t      maxCandidates           = numInputs;
  // Set the type of the buffer
  mBuff->typeBuffer = GPU_KMER_FILTER;
  // Set real size of the input
  mBuff->data.kmer.maxCandidates    = maxCandidates;
  mBuff->data.kmer.maxAlignments    = maxCandidates;
  mBuff->data.kmer.maxBases         = (maxCandidates / candidatesPerQuery) * averageQuerySize;
  mBuff->data.kmer.maxQueries       = (maxCandidates / candidatesPerQuery);
  // Set the corresponding buffer layout
  gpu_kmer_reallocate_host_buffer_layout(mBuff);
  gpu_kmer_reallocate_device_buffer_layout(mBuff);
}

void gpu_kmer_init_and_realloc_buffer_(void *kmerBuffer, const uint32_t totalBases, const uint32_t totalCandidates, const uint32_t totalQueries)
{
  // Buffer re-initialization
  gpu_buffer_t* const mBuff = (gpu_buffer_t *) kmerBuffer;
  const uint32_t averageQuerySize       = totalBases / totalQueries;
  const uint32_t candidatesPerQuery     = totalCandidates / totalQueries;
  // Re-map the buffer layout with new information trying to fit better
  gpu_kmer_init_buffer_(kmerBuffer, averageQuerySize, candidatesPerQuery);
  // Checking if we need to reallocate a bigger buffer
  if( (totalBases      > gpu_kmer_buffer_get_max_qry_bases_(kmerBuffer))   &&
      (totalCandidates > gpu_kmer_buffer_get_max_candidates_(kmerBuffer))  &&
      (totalQueries    > gpu_kmer_buffer_get_max_queries_(kmerBuffer))){
    // Resize the GPU buffer to fit the required input
    const uint32_t  idSupDevice             = mBuff->idSupportedDevice;
    const float     resizeFactor            = 2.0;
    const size_t    bytesPerKmerBuffer      = totalCandidates * gpu_kmer_size_per_candidate(averageQuerySize, candidatesPerQuery);
    //Recalculate the minimum buffer size
    mBuff->sizeBuffer = bytesPerKmerBuffer * resizeFactor;
    //FREE HOST AND DEVICE BUFFER
    GPU_ERROR(gpu_buffer_free(mBuff));
    //Select the device of the Multi-GPU platform
    CUDA_ERROR(cudaSetDevice(mBuff->device[idSupDevice]->idDevice));
    //ALLOCATE HOST AND DEVICE BUFFER
    CUDA_ERROR(cudaHostAlloc((void**) &mBuff->h_rawData, mBuff->sizeBuffer, cudaHostAllocMapped));
    CUDA_ERROR(cudaMalloc((void**) &mBuff->d_rawData, mBuff->sizeBuffer));
    // Remap the buffer layout with the new size
    gpu_kmer_init_buffer_(kmerBuffer, averageQuerySize, candidatesPerQuery);
  }
}

/************************************************************
Functions to send & process a K-MER buffer to GPU
************************************************************/

gpu_error_t gpu_kmer_transfer_CPU_to_GPU(gpu_buffer_t *mBuff)
{
  gpu_kmer_queries_buffer_t    *qry      = &mBuff->data.kmer.queries;
  gpu_kmer_candidates_buffer_t *cand     = &mBuff->data.kmer.candidates;
  gpu_kmer_alignments_buffer_t *res      = &mBuff->data.kmer.alignments;
  cudaStream_t                 idStream  = mBuff->listStreams[mBuff->idStream];
  size_t                       cpySize   = 0;
  float                        bufferUtilization;
  // Defining buffer offsets
  cpySize += qry->numBases * sizeof(gpu_kmer_qry_entry_t);
  cpySize += qry->numQueries * sizeof(gpu_kmer_qry_info_t);
  cpySize += cand->numCandidates * sizeof(gpu_kmer_cand_info_t);
  cpySize += res->numAlignments * sizeof(gpu_kmer_alg_entry_t);
  bufferUtilization = (double)cpySize / (double)mBuff->sizeBuffer;
  // Compacting transference with high buffer occupation
  if(bufferUtilization > 0.15){
    cpySize  = ((void *) (cand->d_candidates + cand->numCandidates)) - ((void *) qry->d_queries);
    CUDA_ERROR(cudaMemcpyAsync(qry->d_queries, qry->h_queries, cpySize, cudaMemcpyHostToDevice, idStream));
  }else{
    // Transfer Binary Queries to GPU
    cpySize = qry->numBases * sizeof(gpu_kmer_qry_entry_t);
    CUDA_ERROR(cudaMemcpyAsync(qry->d_queries, qry->h_queries, cpySize, cudaMemcpyHostToDevice, idStream));
    // Transfer to GPU the information associated with Binary Queries
    cpySize = qry->numQueries * sizeof(gpu_kmer_qry_info_t);
    CUDA_ERROR(cudaMemcpyAsync(qry->d_queryInfo, qry->h_queryInfo, cpySize, cudaMemcpyHostToDevice, idStream));
    // Transfer Candidates to GPU
    cpySize = cand->numCandidates * sizeof(gpu_kmer_cand_info_t);
    CUDA_ERROR(cudaMemcpyAsync(cand->d_candidates, cand->h_candidates, cpySize, cudaMemcpyHostToDevice, idStream));
  }
  // Suceed
  return (SUCCESS);
}

gpu_error_t gpu_kmer_transfer_GPU_to_CPU(gpu_buffer_t *mBuff)
{
  cudaStream_t                 idStream  =  mBuff->listStreams[mBuff->idStream];
  gpu_kmer_alignments_buffer_t *res      = &mBuff->data.kmer.alignments;
  size_t                       cpySize;
  // Transfer Candidates to CPU
  cpySize = res->numAlignments * sizeof(gpu_kmer_alg_entry_t);
  CUDA_ERROR(cudaMemcpyAsync(res->h_alignments, res->d_alignments, cpySize, cudaMemcpyDeviceToHost, idStream));
  // Suceed
  return (SUCCESS);
}

void gpu_kmer_send_buffer_(void* const kmerBuffer, const uint32_t numBases, const uint32_t numQueries,
                           const uint32_t numCandidates, const uint32_t maxError)
{
  gpu_buffer_t* const mBuff       = (gpu_buffer_t *) kmerBuffer;
  const uint32_t      idSupDevice = mBuff->idSupportedDevice;
  //Set real size of the things
  mBuff->data.kmer.queries.numBases            = numBases;
  mBuff->data.kmer.queries.numQueries          = numQueries;
  mBuff->data.kmer.candidates.numCandidates    = numCandidates;
  mBuff->data.kmer.alignments.numAlignments    = numCandidates;
  mBuff->data.kmer.maxError                    = maxError;
  //Select the device of the Multi-GPU platform
  CUDA_ERROR(cudaSetDevice(mBuff->device[idSupDevice]->idDevice));
  //CPU->GPU Transfers & Process Kernel in Asynchronous way
  GPU_ERROR(gpu_kmer_transfer_CPU_to_GPU(mBuff));
  /* INCLUDED SUPPORT for future GPUs with PTX ASM code (JIT compiling) */
  GPU_ERROR(gpu_kmer_process_buffer(mBuff));
  //GPU->CPU Transfers
  GPU_ERROR(gpu_kmer_transfer_GPU_to_CPU(mBuff));
}

/************************************************************
Functions to receive & process a K-MER buffer from GPU
************************************************************/

void gpu_kmer_receive_buffer_(void* const kmerBuffer)
{
  gpu_buffer_t* const mBuff       = (gpu_buffer_t *) kmerBuffer;
  const uint32_t      idSupDevice = mBuff->idSupportedDevice;
  const cudaStream_t  idStream    =  mBuff->listStreams[mBuff->idStream];
  uint32_t i = 0;
  gpu_kmer_alignments_buffer_t *res      = &mBuff->data.kmer.alignments;

  //Select the device of the Multi-GPU platform
  CUDA_ERROR(cudaSetDevice(mBuff->device[idSupDevice]->idDevice));
  //Synchronize Stream (the thread wait for the commands done in the stream)
  CUDA_ERROR(cudaStreamSynchronize(idStream));
}

#endif /* GPU_KMER_PRIMITIVES_C_ */
