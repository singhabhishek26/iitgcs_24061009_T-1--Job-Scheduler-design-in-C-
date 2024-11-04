#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <iomanip>
#include <climits>

const int NUM_WORKER_NODES = 128;
const int CORES_PER_NODE = 24;
const int RAM_PER_NODE = 64;

struct Job
{
  int jobId, arrivalDay, arrivalHour, memoryRequired, coresRequired, executionTime;

  Job(int id, int day, int hour, int mem, int cores, int time)
      : jobId(id), arrivalDay(day), arrivalHour(hour), memoryRequired(mem), coresRequired(cores), executionTime(time) {}

  int getGrossValue() const
  {
    return executionTime * coresRequired * memoryRequired;
  }
};

class WorkerNode
{
public:
  int nodeId, availableCores, availableMemory, totalUsedCores = 0, totalUsedMemory = 0;

  WorkerNode(int id) : nodeId(id), availableCores(CORES_PER_NODE), availableMemory(RAM_PER_NODE) {}

  bool allocateResources(int cores, int memory)
  {
    if (availableCores >= cores && availableMemory >= memory)
    {
      availableCores -= cores;
      availableMemory -= memory;
      totalUsedCores += cores;
      totalUsedMemory += memory;
      return true;
    }
    return false;
  }

  void releaseResources(int cores, int memory)
  {
    availableCores += cores;
    availableMemory += memory;
    totalUsedCores -= cores;
    totalUsedMemory -= memory;
  }
};

class MasterNode
{
private:
  std::vector<WorkerNode> workers;
  std::queue<Job> jobQueue;
  std::string queuePolicy, nodePolicy;

  void queueJob(const Job &job)
  {
    jobQueue.push(job);
    std::cout << "Job " << job.jobId << " is queued\n";
  }

  int findWorkerForJob(const Job &job)
  {
    if (nodePolicy == "first_fit")
    {
      for (auto &worker : workers)
      {
        if (worker.allocateResources(job.coresRequired, job.memoryRequired))
        {
          return worker.nodeId;
        }
      }
    }

    return -1;
  }

public:
  MasterNode(int numWorkers, const std::string &qPolicy, const std::string &nPolicy)
      : queuePolicy(qPolicy), nodePolicy(nPolicy)
  {
    for (int i = 0; i < numWorkers; ++i)
      workers.emplace_back(i);
  }

  void addJob(const Job &job)
  {
    queueJob(job);
  }

  void simulateScheduling()
  {
    std::vector<Job> unassignedJobs;

    while (!jobQueue.empty())
    {
      Job job = jobQueue.front();
      jobQueue.pop();

      int workerId = findWorkerForJob(job);

      if (workerId == -1)
      {

        bool feasible = false;
        for (const auto &worker : workers)
        {
          if (worker.availableCores >= job.coresRequired && worker.availableMemory >= job.memoryRequired)
          {
            feasible = true;
            break;
          }
        }

        if (feasible)
        {

          std::cout << "Job " << job.jobId << " Not assigned, Trying to re-queue\n";
          jobQueue.push(job);
        }
        else
        {

          std::cout << "Job " << job.jobId << " cannot be assigned to any worker because resource unavailability\n";
          unassignedJobs.push_back(job);
        }
      }
      else
      {

        std::cout << "Job " << job.jobId << " assigned to Worker " << workerId << "\n";
      }
    }

    if (!unassignedJobs.empty())
    {
      std::cout << "\nUnassigned Jobs:\n";
      for (const auto &job : unassignedJobs)
      {
        std::cout << "Job " << job.jobId << " could not be scheduled because of insufficient work resource \n";
      }
    }
  }

  void generateCSVReport(const std::string &filename) const
  {
    std::ofstream csvFile(filename);
    if (!csvFile.is_open())
    {
      std::cerr << "Error: Could not open file " << filename << " for writing.\n";
      return;
    }

    csvFile << "Worker ID,Average CPU Utilization (%),Average Memory Utilization (%)\n";
    for (const auto &worker : workers)
    {
      double avgCpuUtil = 100.0 * (CORES_PER_NODE - worker.availableCores) / CORES_PER_NODE;
      double avgMemUtil = 100.0 * (RAM_PER_NODE - worker.availableMemory) / RAM_PER_NODE;
      csvFile << worker.nodeId << "," << avgCpuUtil << "," << avgMemUtil << "\n";
    }
    csvFile.close();
    std::cout << "CSV report generated at: " << filename << "\n";
  }
};

Job parseJobLine(const std::string &line)
{
  std::istringstream iss(line);
  std::string token;
  int jobId, arrivalDay, arrivalHour, memoryRequired, coresRequired, executionTime;

  iss >> token >> jobId >> token >> arrivalDay >> token >> token >> arrivalHour >> token >> memoryRequired >> token >> coresRequired >> token >> executionTime;
  return Job(jobId, arrivalDay, arrivalHour, memoryRequired, coresRequired, executionTime);
}

std::vector<Job> readJobsFromFile(const std::string &filename)
{
  std::vector<Job> jobs;
  std::ifstream file(filename);
  if (!file.is_open())
  {
    std::cerr << "Error: Could not open file " << filename << "\n";
    return jobs;
  }
  std::string line;
  while (std::getline(file, line))
  {
    if (!line.empty())
      jobs.push_back(parseJobLine(line));
  }
  file.close();
  return jobs;
}

int main()
{
  std::string filename = "input_data.txt";
  std::vector<Job> jobs = readJobsFromFile(filename);

  if (jobs.empty())
  {
    std::cerr << "No jobs to schedule. Exiting.\n";
    return 1;
  }

  MasterNode scheduler(NUM_WORKER_NODES, "FCFS", "first_fit");
  for (const auto &job : jobs)
    scheduler.addJob(job);

  scheduler.simulateScheduling();
  scheduler.generateCSVReport("jobs_schedular_report.csv");

  return 0;
}
