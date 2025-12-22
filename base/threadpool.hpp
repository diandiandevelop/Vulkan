/*
* Basic C++11 based thread pool with per-thread job queues
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

// make_unique is not available in C++11
// Taken from Herb Sutter's blog (https://herbsutter.com/gotw/_102/)
// make_unique 在 C++11 中不可用
// 取自 Herb Sutter 的博客 (https://herbsutter.com/gotw/_102/)
/**
 * @brief C++11 兼容的 make_unique 实现
 * @tparam T 要创建的对象类型
 * @tparam Args 构造函数参数类型（可变参数）
 * @param args 传递给构造函数的参数（完美转发）
 * @return 指向新创建对象的 unique_ptr
 */
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));  // 使用完美转发创建对象
}

namespace vks
{
	/**
	 * @brief 线程类，包含每个线程的工作队列
	 */
	class Thread
	{
	private:
		bool destroying = false;                              // 是否正在销毁
		std::thread worker;                                  // 工作线程
		std::queue<std::function<void()>> jobQueue;          // 任务队列
		std::mutex queueMutex;                               // 队列互斥锁
		std::condition_variable condition;                   // 条件变量

		// Loop through all remaining jobs
		// 循环处理所有剩余任务
		/**
		 * @brief 队列循环，从队列中取出任务并执行
		 */
		void queueLoop()
		{
			while (true)
			{
				std::function<void()> job;  // 任务函数对象
				{
					// 等待队列中有任务或线程正在销毁
					std::unique_lock<std::mutex> lock(queueMutex);  // 获取互斥锁
					condition.wait(lock, [this] { return !jobQueue.empty() || destroying; });  // 等待条件满足
					if (destroying)  // 如果正在销毁
					{
						break;  // 退出循环
					}
					job = jobQueue.front();  // 获取队列前端任务
				}

				job();  // 执行任务

				{
					// 任务执行完成后，从队列中移除并通知等待的线程
					std::lock_guard<std::mutex> lock(queueMutex);  // 获取互斥锁
					jobQueue.pop();  // 移除已完成的任务
					condition.notify_one();  // 通知一个等待的线程
				}
			}
		}

	public:
		/**
		 * @brief 构造函数，启动工作线程
		 */
		Thread()
		{
			worker = std::thread(&Thread::queueLoop, this);  // 创建线程并启动队列循环
		}

		/**
		 * @brief 析构函数，等待所有任务完成并停止线程
		 */
		~Thread()
		{
			if (worker.joinable())  // 如果线程可连接
			{
				wait();  // 等待所有任务完成
				queueMutex.lock();  // 获取互斥锁
				destroying = true;  // 设置销毁标志
				condition.notify_one();  // 通知工作线程退出
				queueMutex.unlock();  // 释放互斥锁
				worker.join();  // 等待线程结束
			}
		}

		// Add a new job to the thread's queue
		// 向线程队列添加新任务
		/**
		 * @brief 添加新任务到线程队列
		 * @param function 要执行的任务函数
		 */
		void addJob(std::function<void()> function)
		{
			std::lock_guard<std::mutex> lock(queueMutex);  // 获取互斥锁
			jobQueue.push(std::move(function));  // 将任务添加到队列（移动语义）
			condition.notify_one();  // 通知一个等待的线程有新任务
		}

		// Wait until all work items have been finished
		// 等待直到所有工作项完成
		/**
		 * @brief 等待直到所有任务完成
		 */
		void wait()
		{
			std::unique_lock<std::mutex> lock(queueMutex);  // 获取互斥锁
			condition.wait(lock, [this]() { return jobQueue.empty(); });  // 等待队列为空
		}
	};
	
	/**
	 * @brief 线程池类，管理多个工作线程
	 */
	class ThreadPool
	{
	public:
		std::vector<std::unique_ptr<Thread>> threads;  // 线程列表

		// Sets the number of threads to be allocated in this pool
		// 设置此池中要分配的线程数
		/**
		 * @brief 设置线程池中的线程数量
		 * @param count 线程数量
		 */
		void setThreadCount(uint32_t count)
		{
			threads.clear();  // 清空现有线程
			for (uint32_t i = 0; i < count; i++)  // 创建指定数量的线程
			{
				threads.push_back(make_unique<Thread>());  // 添加新线程到线程列表
			}
		}

		// Wait until all threads have finished their work items
		// 等待直到所有线程完成其工作项
		/**
		 * @brief 等待所有线程完成工作
		 */
		void wait()
		{
			for (auto &thread : threads)  // 遍历所有线程
			{
				thread->wait();  // 等待每个线程完成所有任务
			}
		}
	};

}
