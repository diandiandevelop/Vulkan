# Multithreading 案例分析文档

## 目录
1. [概述](#概述)
2. [多线程渲染架构](#多线程渲染架构)
3. [核心数据结构](#核心数据结构)
4. [程序执行流程](#程序执行流程)
5. [命令缓冲区系统](#命令缓冲区系统)
6. [多线程命令录制详解](#多线程命令录制详解)
7. [视锥剔除系统](#视锥剔除系统)
8. [关键技术点总结](#关键技术点总结)

---

## 概述

`multithreading` 案例展示了 Vulkan 多线程命令缓冲区生成的最佳实践。该案例通过使用线程池并行生成次级命令缓冲区，实现了高效的渲染命令录制，适合渲染大量对象的场景。

### 核心特点

- **多线程并行录制**：使用线程池并行生成命令缓冲区
- **每个线程独立命令池**：每个工作线程拥有自己的命令池，避免线程同步问题
- **次级命令缓冲区**：使用次级命令缓冲区支持并行录制
- **视锥剔除优化**：在每个线程中进行视锥剔除，减少无效绘制
- **Push Constants**：使用 Push Constants 避免描述符集更新开销

### 渲染场景

- 总共渲染 **512 个** UFO 模型对象
- 每个线程负责约 **512/numThreads** 个对象
- 支持多帧并发渲染（maxConcurrentFrames = 2）
- 每个对象都有独立的动画（旋转、位置、缩放）

---

## 多线程渲染架构

### 整体架构图

```mermaid
graph TB
    subgraph "主线程 (Main Thread)"
        Main[主线程<br/>VulkanExample]
        Prepare[prepare<br/>初始化]
        UpdateCmd[updateCommandBuffer<br/>主命令缓冲区]
        Render[render<br/>渲染循环]
    end
    
    subgraph "线程池 (ThreadPool)"
        TP[ThreadPool<br/>线程池管理器]
        T0[Thread 0<br/>工作线程]
        T1[Thread 1<br/>工作线程]
        T2[Thread 2<br/>工作线程]
        TN[Thread N<br/>工作线程<br/>N = hardware_concurrency]
    end
    
    subgraph "每线程数据结构 (ThreadData)"
        TD0[ThreadData 0]
        TD1[ThreadData 1]
        TD2[ThreadData 2]
        TDN[ThreadData N]
    end
    
    subgraph "命令缓冲区层次"
        Primary[主命令缓冲区<br/>Primary Command Buffer<br/>drawCmdBuffers]
        
        subgraph "次级命令缓冲区 (Secondary)"
            SecBG[背景命令缓冲区<br/>secondaryCommandBuffers<br/>background]
            SecUI[UI命令缓冲区<br/>secondaryCommandBuffers<br/>ui]
            SecT0["Thread 0 次级命令缓冲区<br/>threadData(0).commandBuffer<br/>numObjectsPerThread个"]
            SecT1["Thread 1 次级命令缓冲区<br/>threadData(1).commandBuffer"]
            SecTN[Thread N 次级命令缓冲区]
        end
    end
    
    Main --> Prepare
    Prepare --> TP
    TP --> T0
    TP --> T1
    TP --> T2
    TP --> TN
    
    T0 --> TD0
    T1 --> TD1
    T2 --> TD2
    TN --> TDN
    
    TD0 --> SecT0
    TD1 --> SecT1
    TDN --> SecTN
    
    Render --> UpdateCmd
    UpdateCmd --> Primary
    UpdateCmd --> SecBG
    UpdateCmd --> SecUI
    UpdateCmd --> T0
    UpdateCmd --> T1
    UpdateCmd --> TN
    
    T0 --> SecT0
    T1 --> SecT1
    TN --> SecTN
    
    Primary -->|vkCmdExecuteCommands| SecBG
    Primary -->|vkCmdExecuteCommands| SecUI
    Primary -->|vkCmdExecuteCommands| SecT0
    Primary -->|vkCmdExecuteCommands| SecT1
    Primary -->|vkCmdExecuteCommands| SecTN
    
    style Main fill:#FFE4B5
    style TP fill:#87CEEB
    style Primary fill:#DDA0DD
    style SecBG fill:#90EE90
    style SecUI fill:#90EE90
```

### 多线程命令录制架构

```mermaid
graph LR
    subgraph "主线程"
        Begin[开始录制<br/>vkBeginCommandBuffer]
        BeginRP[开始渲染通道<br/>vkCmdBeginRenderPass<br/>VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS]
        AddJobs[分发任务到线程池<br/>addJob]
        Wait[等待所有线程完成<br/>threadPool.wait]
        Collect[收集可见对象的命令缓冲区]
        Execute[执行所有次级命令缓冲区<br/>vkCmdExecuteCommands]
        EndRP[结束渲染通道<br/>vkCmdEndRenderPass]
        End[结束录制<br/>vkEndCommandBuffer]
    end
    
    subgraph "线程池并行执行"
        T0[Thread 0<br/>threadRenderCode]
        T1[Thread 1<br/>threadRenderCode]
        T2[Thread 2<br/>threadRenderCode]
        TN[Thread N<br/>threadRenderCode]
    end
    
    subgraph "每线程工作流程"
        CheckVis[视锥剔除检查<br/>frustum.checkSphere]
        BeginSec[开始次级命令缓冲区<br/>vkBeginCommandBuffer]
        Record[录制渲染命令<br/>vkCmdSetViewport<br/>vkCmdBindPipeline<br/>vkCmdPushConstants<br/>vkCmdDrawIndexed]
        EndSec[结束次级命令缓冲区<br/>vkEndCommandBuffer]
    end
    
    Begin --> BeginRP
    BeginRP --> AddJobs
    AddJobs --> T0
    AddJobs --> T1
    AddJobs --> T2
    AddJobs --> TN
    
    T0 --> CheckVis
    T1 --> CheckVis
    T2 --> CheckVis
    TN --> CheckVis
    
    CheckVis -->|可见| BeginSec
    BeginSec --> Record
    Record --> EndSec
    CheckVis -->|不可见| Wait
    
    T0 -.->|完成| Wait
    T1 -.->|完成| Wait
    T2 -.->|完成| Wait
    TN -.->|完成| Wait
    
    Wait --> Collect
    Collect --> Execute
    Execute --> EndRP
    EndRP --> End
    
    style Begin fill:#90EE90
    style Wait fill:#FFB6C1
    style Execute fill:#DDA0DD
```

---

## 核心数据结构

### ThreadData 结构

每个工作线程都有独立的数据结构，避免线程间的数据竞争：

```mermaid
classDiagram
    class ThreadData {
        +VkCommandPool commandPool
        +array~vector~VkCommandBuffer~~ commandBuffer[maxConcurrentFrames]
        +vector~ThreadPushConstantBlock~ pushConstBlock
        +vector~ObjectData~ objectData
    }
    
    class ThreadPushConstantBlock {
        +glm::mat4 mvp
        +glm::vec3 color
    }
    
    class ObjectData {
        +glm::mat4 model
        +glm::vec3 pos
        +glm::vec3 rotation
        +float rotationDir
        +float rotationSpeed
        +float scale
        +float deltaT
        +float stateT
        +bool visible
    }
    
    ThreadData "1" *-- "N" ThreadPushConstantBlock : contains
    ThreadData "1" *-- "N" ObjectData : contains
    ThreadData "1" *-- "1" VkCommandPool : owns
    ThreadData "1" *-- "M" VkCommandBuffer : owns
```

### 数据结构关系图

```mermaid
graph TB
    subgraph "VulkanExample 主类"
        ET[VulkanExample]
        TP[ThreadPool<br/>threadPool]
        TD[ThreadData数组<br/>threadData<br/>size = numThreads]
        Matrices[共享矩阵<br/>matrices<br/>projection + view]
        Frustum[视锥对象<br/>frustum]
    end
    
    subgraph "ThreadData(0)"
        TD0[ThreadData 0]
        CP0["CommandPool 0<br/>threadData(0).commandPool"]
        CB0["CommandBuffer数组<br/>threadData(0).commandBuffer<br/>(frame0)(frame1) x numObjectsPerThread"]
        PC0["PushConstant数组<br/>threadData(0).pushConstBlock<br/>size = numObjectsPerThread"]
        OD0["ObjectData数组<br/>threadData(0).objectData<br/>size = numObjectsPerThread"]
    end
    
    subgraph "ThreadData(N)"
        TDN[ThreadData N]
        CPN[CommandPool N]
        CBN["CommandBuffer数组<br/>threadData(N).commandBuffer"]
        PCN[PushConstant数组]
        ODN[ObjectData数组]
    end
    
    ET --> TP
    ET --> TD
    ET --> Matrices
    ET --> Frustum
    
    TD --> TD0
    TD --> TDN
    
    TD0 --> CP0
    TD0 --> CB0
    TD0 --> PC0
    TD0 --> OD0
    
    TDN --> CPN
    TDN --> CBN
    TDN --> PCN
    TDN --> ODN
    
    TP -.->|线程执行| TD0
    TP -.->|线程执行| TDN
    
    Matrices -.->|读取| TD0
    Matrices -.->|读取| TDN
    Frustum -.->|剔除测试| OD0
    Frustum -.->|剔除测试| ODN
    
    style ET fill:#FFE4B5
    style TP fill:#87CEEB
    style CP0 fill:#DDA0DD
    style CPN fill:#DDA0DD
```

---

## 程序执行流程

### 初始化流程

```mermaid
flowchart TD
    Start([程序启动]) --> Constructor[VulkanExample构造函数]
    
    Constructor --> GetThreads[获取硬件线程数<br/>numThreads = hardware_concurrency]
    GetThreads --> SetupPool[创建线程池<br/>threadPool.setThreadCount<br/>numThreads]
    SetupPool --> CalcObjects[计算每线程对象数<br/>numObjectsPerThread = 512 / numThreads]
    
    CalcObjects --> InitVulkan[基类初始化<br/>VulkanExampleBase::prepare]
    
    subgraph "VulkanExampleBase::prepare"
        InitVulkan --> CreateInstance[创建Vulkan实例]
        CreateInstance --> CreateDevice[创建设备和队列]
        CreateDevice --> CreateSwapChain[创建交换链]
        CreateSwapChain --> CreateCmdPool[创建主命令池<br/>cmdPool]
        CreateCmdPool --> CreateBaseSync[创建同步对象]
        CreateBaseSync --> SetupDepth[设置深度缓冲区]
        SetupDepth --> SetupRenderPass[创建渲染通道]
        SetupRenderPass --> CreateFrameBuffers[创建帧缓冲区]
    end
    
    CreateFrameBuffers --> Prepare[派生类prepare]
    
    subgraph "VulkanExample::prepare"
        Prepare --> LoadAssets[加载模型<br/>loadAssets<br/>- ufo模型<br/>- starSphere模型]
        LoadAssets --> PreparePipelines[准备管线<br/>preparePipelines<br/>- phong管线<br/>- starsphere管线]
        PreparePipelines --> PrepareMT[准备多线程渲染器<br/>prepareMultiThreadedRenderer]
        
        subgraph "prepareMultiThreadedRenderer 详细流程"
            PrepareMT --> AllocSecBG[分配背景次级命令缓冲区<br/>secondaryCommandBuffers<br/>background]
            AllocSecBG --> AllocSecUI[分配UI次级命令缓冲区<br/>secondaryCommandBuffers<br/>ui]
            AllocSecUI --> ResizeThreadData[调整线程数据数组<br/>threadData.resize numThreads]
            
            ResizeThreadData --> LoopThreads[循环创建每个线程数据]
            
            subgraph "为每个线程创建资源"
                LoopThreads --> CreateCmdPool[创建线程命令池<br/>vkCreateCommandPool<br/>每个线程独立]
                CreateCmdPool --> AllocCmdBufs[分配次级命令缓冲区<br/>vkAllocateCommandBuffers<br/>每个线程: maxConcurrentFrames x numObjectsPerThread个]
                AllocCmdBufs --> ResizePC[调整PushConstant数组<br/>pushConstBlock.resize]
                ResizePC --> ResizeOD[调整ObjectData数组<br/>objectData.resize]
                ResizeOD --> InitObjects[初始化对象数据<br/>随机位置/旋转/缩放/颜色]
            end
            
            InitObjects --> EndPrepareMT[完成多线程准备]
        end
        
        EndPrepareMT --> UpdateMatrices[更新矩阵<br/>updateMatrices]
        UpdateMatrices --> SetPrepared[prepared = true]
    end
    
    SetPrepared --> End([初始化完成])
    
    style Start fill:#90EE90
    style End fill:#90EE90
    style PrepareMT fill:#FFB6C1
    style CreateCmdPool fill:#DDA0DD
```

### 渲染循环流程

```mermaid
flowchart TD
    Start([render函数开始]) --> CheckPrep{prepared?}
    CheckPrep -->|否| Return([直接返回])
    CheckPrep -->|是| PrepareFrame[基类prepareFrame<br/>- 等待栅栏<br/>- 获取交换链图像<br/>- 重置栅栏]
    
    PrepareFrame --> UpdateCmd[updateCommandBuffer<br/>更新命令缓冲区]
    
    subgraph "updateCommandBuffer 详细流程"
        UpdateCmd --> BeginPrimary["开始主命令缓冲区<br/>vkBeginCommandBuffer<br/>drawCmdBuffers(currentBuffer)"]
        
        BeginPrimary --> SetupRP["设置渲染通道信息<br/>VkRenderPassBeginInfo<br/>- renderPass<br/>- framebuffer(currentImageIndex)<br/>- clearValues"]
        
        SetupRP --> BeginRP[开始渲染通道<br/>vkCmdBeginRenderPass<br/>VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS]
        
        BeginRP --> SetupInherit[设置继承信息<br/>VkCommandBufferInheritanceInfo<br/>- renderPass<br/>- framebuffer]
        
        SetupInherit --> UpdateSec[更新场景次级命令缓冲区<br/>updateSecondaryCommandBuffers<br/>- 背景命令缓冲区<br/>- UI命令缓冲区]
        
        UpdateSec --> AddBackground{显示背景?}
        AddBackground -->|是| PushBG["添加背景命令缓冲区<br/>commandBuffers.push_back<br/>secondaryCommandBuffers(currentBuffer).background"]
        AddBackground -->|否| DistributeJobs
        
        PushBG --> DistributeJobs["分发任务到线程池<br/>for each thread:<br/>  for each object:<br/>    threadPool.threads(t)->addJob<br/>    threadRenderCode(t, i, inheritanceInfo)"]
        
        DistributeJobs --> WaitThreads[等待所有线程完成<br/>threadPool.wait]
        
        WaitThreads --> CollectVisible["收集可见对象的命令缓冲区<br/>for each thread:<br/>  for each object:<br/>    if objectData.visible:<br/>      commandBuffers.push_back<br/>      commandBuffer(currentBuffer)(i)"]
        
        CollectVisible --> AddUI{UI可见?}
        AddUI -->|是| PushUI["添加UI命令缓冲区<br/>commandBuffers.push_back<br/>secondaryCommandBuffers(currentBuffer).ui"]
        AddUI -->|否| Execute
        
        PushUI --> Execute["执行所有次级命令缓冲区<br/>vkCmdExecuteCommands<br/>drawCmdBuffers(currentBuffer)<br/>commandBuffers数组"]
        
        Execute --> EndRP[结束渲染通道<br/>vkCmdEndRenderPass]
        
        EndRP --> EndPrimary[结束主命令缓冲区<br/>vkEndCommandBuffer]
    end
    
    EndPrimary --> UpdateMatrices[updateMatrices<br/>更新投影和视图矩阵<br/>更新视锥]
    
    UpdateMatrices --> SubmitFrame[基类submitFrame<br/>- 提交到队列<br/>- 呈现图像<br/>- 更新currentFrame]
    
    SubmitFrame --> End([render函数结束])
    
    style Start fill:#90EE90
    style End fill:#90EE90
    style DistributeJobs fill:#FFB6C1
    style WaitThreads fill:#87CEEB
    style Execute fill:#DDA0DD
```

### 多线程命令录制详细流程

```mermaid
sequenceDiagram
    participant Main as 主线程
    participant TP as 线程池
    participant T0 as 线程0
    participant T1 as 线程1
    participant TN as 线程N
    participant GPU as GPU队列
    
    Main->>Main: 开始主命令缓冲区
    Main->>Main: 开始渲染通道<br/>VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    Main->>Main: 准备继承信息
    
    Main->>TP: 分发任务<br/>addJob(threadRenderCode)
    
    par 并行执行
        TP->>T0: 执行 threadRenderCode(0, 0)
        T0->>T0: 视锥剔除检查
        alt 对象可见
            T0->>T0: 开始次级命令缓冲区
            T0->>T0: 设置视口和裁剪
            T0->>T0: 绑定管线
            T0->>T0: 更新对象动画
            T0->>T0: 计算模型矩阵
            T0->>T0: 计算MVP矩阵
            T0->>T0: 推送PushConstants
            T0->>T0: 绑定顶点/索引缓冲区
            T0->>T0: 绘制命令 vkCmdDrawIndexed
            T0->>T0: 结束次级命令缓冲区
        end
        
        TP->>T1: 执行 threadRenderCode(1, 0)
        T1->>T1: 视锥剔除检查
        T1->>T1: 录制命令...
        
        TP->>TN: 执行 threadRenderCode(N, i)
        TN->>TN: 视锥剔除检查
        TN->>TN: 录制命令...
    end
    
    Main->>TP: 等待所有任务完成<br/>threadPool.wait
    TP->>Main: 所有线程完成
    
    Main->>Main: 收集可见对象的命令缓冲区
    Main->>Main: 执行所有次级命令缓冲区<br/>vkCmdExecuteCommands
    Main->>Main: 结束渲染通道
    Main->>Main: 结束主命令缓冲区
    
    Main->>GPU: 提交命令缓冲区<br/>vkQueueSubmit
    GPU->>GPU: GPU执行渲染
```

---

## 命令缓冲区系统

### 命令缓冲区层次结构

```mermaid
graph TB
    subgraph "主命令缓冲区 (Primary)"
        Primary["主命令缓冲区<br/>drawCmdBuffers(currentBuffer)<br/>VK_COMMAND_BUFFER_LEVEL_PRIMARY"]
    end
    
    subgraph "次级命令缓冲区 (Secondary) - 场景"
        SecBG["背景命令缓冲区<br/>secondaryCommandBuffers(currentBuffer).background<br/>VK_COMMAND_BUFFER_LEVEL_SECONDARY"]
        SecUI["UI命令缓冲区<br/>secondaryCommandBuffers(currentBuffer).ui<br/>VK_COMMAND_BUFFER_LEVEL_SECONDARY"]
    end
    
    subgraph "次级命令缓冲区 (Secondary) - 线程0"
        SecT0_0["Thread 0 Object 0<br/>threadData(0).commandBuffer(currentBuffer)(0)"]
        SecT0_1["Thread 0 Object 1<br/>threadData(0).commandBuffer(currentBuffer)(1)"]
        SecT0_N["Thread 0 Object N<br/>threadData(0).commandBuffer(currentBuffer)(numObjectsPerThread-1)"]
    end
    
    subgraph "次级命令缓冲区 (Secondary) - 线程N"
        SecTN_0["Thread N Object 0<br/>threadData(N).commandBuffer(currentBuffer)(0)"]
        SecTN_N["Thread N Object M<br/>threadData(N).commandBuffer(currentBuffer)(numObjectsPerThread-1)"]
    end
    
    Primary -->|vkCmdExecuteCommands| SecBG
    Primary -->|vkCmdExecuteCommands| SecUI
    Primary -->|vkCmdExecuteCommands| SecT0_0
    Primary -->|vkCmdExecuteCommands| SecT0_1
    Primary -->|vkCmdExecuteCommands| SecT0_N
    Primary -->|vkCmdExecuteCommands| SecTN_0
    Primary -->|vkCmdExecuteCommands| SecTN_N
    
    style Primary fill:#FFE4B5
    style SecBG fill:#90EE90
    style SecUI fill:#90EE90
    style SecT0_0 fill:#87CEEB
    style SecTN_0 fill:#87CEEB
```

### 命令缓冲区分配图

```mermaid
graph LR
    subgraph "每帧命令缓冲区结构"
        subgraph "Frame 0"
            F0_Primary["主命令缓冲区<br/>drawCmdBuffers(0)"]
            F0_SecBG["背景Sec<br/>secondaryCommandBuffers(0).background"]
            F0_SecUI["UI Sec<br/>secondaryCommandBuffers(0).ui"]
            F0_T0["Thread 0 Sec数组<br/>threadData(0).commandBuffer(0)<br/>numObjectsPerThread个"]
            F0_TN["Thread N Sec数组<br/>threadData(N).commandBuffer(0)"]
        end
        
        subgraph "Frame 1"
            F1_Primary["主命令缓冲区<br/>drawCmdBuffers(1)"]
            F1_SecBG["背景Sec<br/>secondaryCommandBuffers(1).background"]
            F1_SecUI["UI Sec<br/>secondaryCommandBuffers(1).ui"]
            F1_T0["Thread 0 Sec数组<br/>threadData(0).commandBuffer(1)"]
            F1_TN["Thread N Sec数组<br/>threadData(N).commandBuffer(1)"]
        end
    end
    
    subgraph "命令池层次"
        MainPool[主命令池<br/>cmdPool<br/>基类创建]
        ThreadPool0["线程0命令池<br/>threadData(0).commandPool"]
        ThreadPoolN["线程N命令池<br/>threadData(N).commandPool"]
    end
    
    MainPool --> F0_Primary
    MainPool --> F1_Primary
    MainPool --> F0_SecBG
    MainPool --> F0_SecUI
    MainPool --> F1_SecBG
    MainPool --> F1_SecUI
    
    ThreadPool0 --> F0_T0
    ThreadPool0 --> F1_T0
    ThreadPoolN --> F0_TN
    ThreadPoolN --> F1_TN
    
    style MainPool fill:#DDA0DD
    style ThreadPool0 fill:#87CEEB
    style ThreadPoolN fill:#87CEEB
```

### 命令缓冲区使用时间线

```mermaid
gantt
    title 命令缓冲区录制和执行时间线 (假设4个线程，每线程128个对象)
    dateFormat X
    axisFormat %s
    
    section 主线程
    开始主命令缓冲区 :0, 1s
    开始渲染通道 :1, 1s
    分发任务到线程池 :2, 1s
    等待线程完成 :3, 50s
    收集命令缓冲区 :53, 1s
    执行次级命令缓冲区 :54, 1s
    结束渲染通道 :55, 1s
    提交到GPU :56, 1s
    
    section 线程0
    视锥剔除和录制(128个对象) :2, 25s
    
    section 线程1
    视锥剔除和录制(128个对象) :2, 25s
    
    section 线程2
    视锥剔除和录制(128个对象) :2, 28s
    
    section 线程3
    视锥剔除和录制(128个对象) :2, 30s
    
    section GPU执行
    GPU执行渲染 :57, 10s
```

---

## 多线程命令录制详解

### threadRenderCode 函数详细流程

`threadRenderCode` 是每个线程执行的核心函数，负责为单个对象录制渲染命令：

```mermaid
flowchart TD
    Start([threadRenderCode开始<br/>threadIndex, cmdBufferIndex, inheritanceInfo]) --> GetThread["获取线程数据<br/>thread = threadData(threadIndex)"]
    
    GetThread --> GetObject["获取对象数据<br/>objectData = thread->objectData(cmdBufferIndex)"]
    
    GetObject --> FrustumCull[视锥剔除检查<br/>frustum.checkSphere<br/>objectData->pos, radius]
    
    FrustumCull -->|不可见| SetInvisible[设置可见标志<br/>objectData->visible = false]
    SetInvisible --> Return([直接返回])
    
    FrustumCull -->|可见| SetVisible[设置可见标志<br/>objectData->visible = true]
    
    SetVisible --> SetupBeginInfo[设置命令缓冲区开始信息<br/>VkCommandBufferBeginInfo<br/>- flags: RENDER_PASS_CONTINUE_BIT<br/>- pInheritanceInfo: inheritanceInfo]
    
    SetupBeginInfo --> GetCmdBuf["获取命令缓冲区<br/>cmdBuffer = thread->commandBuffer<br/>(currentBuffer)(cmdBufferIndex)"]
    
    GetCmdBuf --> BeginCmd[开始命令缓冲区<br/>vkBeginCommandBuffer<br/>cmdBuffer]
    
    BeginCmd --> SetViewport[设置视口<br/>vkCmdSetViewport<br/>width x height]
    
    SetViewport --> SetScissor[设置裁剪矩形<br/>vkCmdSetScissor<br/>width x height]
    
    SetScissor --> BindPipeline[绑定图形管线<br/>vkCmdBindPipeline<br/>pipelines.phong]
    
    BindPipeline --> UpdateAnimation{暂停?}
    
    UpdateAnimation -->|否| CalcRotation["计算旋转<br/>rotation.y += rotationSpeed * frameTimer"]
    CalcRotation --> CalcDeltaT["计算deltaT<br/>deltaT += 0.15f * frameTimer"]
    CalcDeltaT --> CalcPosY["计算Y位置<br/>pos.y = sin(deltaT * 360°) * 2.5f"]
    
    CalcPosY --> BuildModel
    UpdateAnimation -->|是| BuildModel["构建模型矩阵<br/>model = translate * rotate * scale"]
    
    BuildModel --> CalcMVP["计算MVP矩阵<br/>mvp = projection * view * model"]
    
    CalcMVP --> PushConstants[推送PushConstants<br/>vkCmdPushConstants<br/>- mvp矩阵<br/>- color颜色]
    
    PushConstants --> BindVertex[绑定顶点缓冲区<br/>vkCmdBindVertexBuffers<br/>models.ufo.vertices.buffer]
    
    BindVertex --> BindIndex[绑定索引缓冲区<br/>vkCmdBindIndexBuffer<br/>models.ufo.indices.buffer]
    
    BindIndex --> Draw[绘制命令<br/>vkCmdDrawIndexed<br/>indices.count个索引]
    
    Draw --> EndCmd[结束命令缓冲区<br/>vkEndCommandBuffer]
    
    EndCmd --> End([函数结束])
    
    style Start fill:#90EE90
    style FrustumCull fill:#FFB6C1
    style BuildModel fill:#87CEEB
    style Draw fill:#DDA0DD
    style End fill:#90EE90
```

### 命令录制内容示例

每个次级命令缓冲区包含以下命令序列：

```
vkBeginCommandBuffer (VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)
  ↓
vkCmdSetViewport
  ↓
vkCmdSetScissor
  ↓
vkCmdBindPipeline (VK_PIPELINE_BIND_POINT_GRAPHICS, phong pipeline)
  ↓
vkCmdPushConstants (mvp矩阵 + color颜色)
  ↓
vkCmdBindVertexBuffers (UFO模型顶点缓冲区)
  ↓
vkCmdBindIndexBuffer (UFO模型索引缓冲区)
  ↓
vkCmdDrawIndexed (绘制索引图元)
  ↓
vkEndCommandBuffer
```

---

## 视锥剔除系统

### 视锥剔除流程图

```mermaid
flowchart TD
    Start([每帧开始]) --> UpdateMatrices["更新矩阵<br/>updateMatrices<br/>- 更新projection<br/>- 更新view<br/>- 更新frustum<br/>frustum.update(projection * view)"]
    
    UpdateMatrices --> ExtractPlanes["提取视锥平面<br/>从projection * view矩阵提取6个平面<br/>- 左平面<br/>- 右平面<br/>- 上平面<br/>- 下平面<br/>- 前平面(近)<br/>- 后平面(远)"]
    
    ExtractPlanes --> NormalizePlanes[归一化平面<br/>归一化每个平面的法线向量]
    
    NormalizePlanes --> ThreadCull[线程执行视锥剔除]
    
    subgraph "每个线程的剔除过程"
        ThreadCull --> ForEachObject[遍历该线程的所有对象]
        ForEachObject --> GetObjectPos[获取对象位置<br/>objectData.pos]
        GetObjectPos --> GetRadius["获取对象半径<br/>models.ufo.dimensions.radius * 0.5f"]
        GetRadius --> CheckPlane[检查每个视锥平面]
        
        subgraph "平面测试循环"
            CheckPlane --> CalcDistance["计算球心到平面的距离<br/>distance = plane.x * pos.x +<br/>plane.y * pos.y +<br/>plane.z * pos.z +<br/>plane.w"]
            CalcDistance --> CompareDistance{distance <= -radius?}
            CompareDistance -->|是| Outside[在视锥外<br/>返回false]
            CompareDistance -->|否| NextPlane{还有平面?}
            NextPlane -->|是| CheckPlane
            NextPlane -->|否| Inside[在视锥内<br/>返回true]
        end
        
        Outside --> SetInvisible[设置visible = false<br/>不录制命令]
        Inside --> SetVisible[设置visible = true<br/>录制命令]
    end
    
    SetInvisible --> NextObject{还有对象?}
    SetVisible --> NextObject
    NextObject -->|是| ForEachObject
    NextObject -->|否| End([剔除完成])
    
    style Start fill:#90EE90
    style UpdateMatrices fill:#FFE4B5
    style CheckPlane fill:#FFB6C1
    style Inside fill:#90EE90
    style Outside fill:#FF6B6B
```

### 视锥剔除原理图

```mermaid
graph TB
    subgraph "视锥体"
        Camera[摄像机位置]
        Near[近裁剪面]
        Far[远裁剪面]
        Left[左平面]
        Right[右平面]
        Top[上平面]
        Bottom[下平面]
    end
    
    subgraph "场景对象"
        Obj1[对象1<br/>在视锥内<br/>visible = true]
        Obj2[对象2<br/>在视锥外<br/>visible = false]
        Obj3[对象3<br/>与视锥相交<br/>visible = true]
    end
    
    Camera --> Near
    Camera --> Far
    Camera --> Left
    Camera --> Right
    Camera --> Top
    Camera --> Bottom
    
    Near -.->|包含| Obj1
    Far -.->|包含| Obj1
    Left -.->|包含| Obj1
    Right -.->|包含| Obj1
    Top -.->|包含| Obj1
    Bottom -.->|包含| Obj1
    
    Left -.->|在外| Obj2
    Right -.->|在外| Obj2
    
    Near -.->|相交| Obj3
    Far -.->|相交| Obj3
    
    style Camera fill:#FFE4B5
    style Obj1 fill:#90EE90
    style Obj2 fill:#FF6B6B
    style Obj3 fill:#FFB6C1
```

### 视锥剔除算法

每个对象使用球体进行视锥剔除测试：

```
算法: checkSphere(pos, radius)
  对于每个视锥平面 plane (共6个):
    计算球心到平面的距离:
      distance = plane.x * pos.x + plane.y * pos.y + plane.z * pos.z + plane.w
    
    如果 distance <= -radius:
      返回 false (球体完全在视锥外)
  
  返回 true (球体在视锥内或与视锥相交)
```

---

## 关键技术点总结

### 1. 多线程命令录制策略

#### 关键设计点

- **每个线程独立命令池**：避免命令池的线程同步开销
  ```cpp
  // 每个线程都有独立的命令池
  VkCommandPoolCreateInfo cmdPoolInfo = ...;
  cmdPoolInfo.queueFamilyIndex = swapChain.queueNodeIndex;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &thread->commandPool);
  ```

- **使用次级命令缓冲区**：支持并行录制，然后在主线程中执行
  ```cpp
  // 次级命令缓冲区标志
  VkCommandBufferBeginInfo commandBufferBeginInfo = ...;
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
  commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
  ```

- **主命令缓冲区使用 SECONDARY 标志**：
  ```cpp
  // 主命令缓冲区开始渲染通道时指定使用次级命令缓冲区
  vkCmdBeginRenderPass(..., VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
  ```

### 2. 线程池实现

#### ThreadPool 架构

```mermaid
classDiagram
    class ThreadPool {
        +vector~unique_ptr~Thread~~ threads
        +setThreadCount(uint32_t count)
        +wait()
    }
    
    class Thread {
        -bool destroying
        -thread worker
        -queue~function~void~~ jobQueue
        -mutex queueMutex
        -condition_variable condition
        +addJob(function~void~ job)
        +wait()
        -queueLoop()
    }
    
    ThreadPool "1" *-- "N" Thread : contains
    
    note for ThreadPool "管理多个工作线程"
    note for Thread "每个线程有独立的任务队列"
```

#### 工作流程

1. **任务分发**：主线程将任务添加到每个线程的队列
2. **并行执行**：每个线程从自己的队列中取任务执行
3. **等待同步**：主线程等待所有线程完成

### 3. Push Constants 使用

使用 Push Constants 而不是 Uniform Buffer，避免描述符集更新：

```cpp
struct ThreadPushConstantBlock {
    glm::mat4 mvp;      // 模型视图投影矩阵
    glm::vec3 color;    // 对象颜色
};

// 每个线程在录制命令时推送自己的常量
vkCmdPushConstants(
    cmdBuffer,
    pipelineLayout,
    VK_SHADER_STAGE_VERTEX_BIT,
    0,
    sizeof(ThreadPushConstantBlock),
    &thread->pushConstBlock[cmdBufferIndex]);
```

**优势**：
- 每个线程可以独立更新，无需同步
- 避免描述符集的频繁更新
- 性能开销小

### 4. 多帧并发支持

每个线程的命令缓冲区数组支持多帧并发：

```cpp
// 每个线程的命令缓冲区数组
std::array<std::vector<VkCommandBuffer>, maxConcurrentFrames> commandBuffer;

// 使用当前帧索引
VkCommandBuffer cmdBuffer = thread->commandBuffer[currentBuffer][cmdBufferIndex];
```

### 5. 性能优化技巧

1. **视锥剔除**：在录制命令前剔除不可见对象，减少 GPU 工作
2. **负载均衡**：对象均匀分配到各线程（512 对象 / 线程数）
3. **避免同步**：每个线程使用独立资源，减少锁竞争
4. **批量执行**：使用 `vkCmdExecuteCommands` 批量执行所有次级命令缓冲区

### 6. 关键数据流

```mermaid
graph LR
    subgraph "共享数据（只读）"
        Matrices[matrices<br/>projection + view]
        Models[models<br/>ufo + starSphere]
        Pipelines[pipelines<br/>phong + starsphere]
        Frustum[frustum<br/>视锥对象]
    end
    
    subgraph "每线程独立数据（读写）"
        ThreadData[ThreadData<br/>- commandPool<br/>- commandBuffer<br/>- pushConstBlock<br/>- objectData]
    end
    
    Matrices -->|读取| ThreadData
    Models -->|读取| ThreadData
    Pipelines -->|读取| ThreadData
    Frustum -->|读取| ThreadData
    
    ThreadData -->|写入| CommandBuffers[命令缓冲区]
    CommandBuffers -->|执行| GPU
    
    style Matrices fill:#FFE4B5
    style ThreadData fill:#87CEEB
    style CommandBuffers fill:#DDA0DD
```

### 7. 线程安全考虑

- ✅ **命令池隔离**：每个线程有独立的命令池，无竞争
- ✅ **命令缓冲区隔离**：每个线程有独立的命令缓冲区，无竞争
- ✅ **对象数据隔离**：每个线程管理自己的对象数组
- ✅ **共享资源只读**：矩阵、模型、管线等共享资源只读访问
- ⚠️ **视锥对象**：虽然被多线程读取，但只在主线程更新，线程安全

### 8. 扩展建议

1. **动态负载均衡**：根据对象复杂度动态分配任务
2. **异步剔除**：在上一帧提前进行视锥剔除
3. **实例化渲染**：对于相同模型，使用实例化减少命令数量
4. **间接绘制**：使用间接绘制命令进一步优化

---

## 总结

`multithreading` 案例展示了 Vulkan 多线程命令缓冲区生成的最佳实践：

1. **架构清晰**：主线程管理，多线程并行录制，最后统一执行
2. **性能优秀**：利用多核 CPU 并行录制命令，显著提升性能
3. **线程安全**：通过资源隔离确保线程安全
4. **可扩展性**：支持任意数量的线程和对象

这个案例是学习 Vulkan 多线程渲染的绝佳参考，展示了如何充分利用 CPU 多核能力来优化渲染性能。

