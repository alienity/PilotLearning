> 练习渲染技术和学习d3d12的渲染器，基于RenderGraph自动生成Pass层级，使用全bindless，将所有的几乎全部计算都在GPU上完成

# 实现模块
1. RenderGraph
2. Bindless
3. GPU Culling
4. GPU Driven Terrain
5. FXAA
6. TAA
7. SSAO
8. HBAO
9. Bloom
10. ToneMapping

## 管线流程
### RenderGraph简介
RenderGraph会生成一个有向无环图，Graph中包含Pass节点和Resource节点，通过在有向无环图中进行拓扑排序，可以找出可以并行录制并且一起做barrier的pass，从而可以得到执行的管线流程图，如下图所示，而重新组织后的graph则会更清晰。

一起执行Barrier可以在pass执行之前很久就把状态转换做好

### 直接根据Graph生成的Graph
![rawImage](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/graph.png)

### 重新组织后的Graph
![groupImage](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/%E4%BC%98%E5%8C%96%E6%B5%81%E7%A8%8B.drawio.png)

## GPU Culling
场景mesh对象一并上传到gpu，通过`OpaqueTransCullingPass`直接使用相机进行剪裁，而对于DirectionLight的Shadowmap也是类似的使用`DirectionLightShadowCullPass`进行剪裁，而对SpotLight则是`SpotLightShadowCullPass`进行剪裁。

### Opaque和Transparent排序
对于Transparent对象则可以使用用`OpaqueBitonicSortPass`和`TransparentBitonicSortPass`在GPU上使用BitonicSort进行排序，BitonicSort一般用在particle的排序上，但是我们既然是全bindless，自然也就将排序也放在gpu上了

### 获取排序后数据并绘制
通过`GrabOpaquePass`和`GrabTransPass`从场景mesh数据中获取要绘制的Opaque和Transparent对象数据，并使用ExecuteIndirect进行绘制，使用ExecuteIndirect可以减少CommandList需要录制的指令数量，一次将所有相同材质对象都绘制上去。

绘制的GBuffer数据:
- ColorBuffer
- WorldNormal
- WorldTangent
- MaterialNormal
- MetallicRoughnessRefectanceAO
- ClearCoatClearCoatRoughnessAnistropy
- MotionVector
- DepthBuffer

![GBuffer_Color](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Color.png)
![GBuffer_Normal](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Normal.png)
![GBuffer_Tangent](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Tangent.png)
![Material_Normal](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/Material_Normal.png)
![GBuffer_MRRA](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_MRRA.png)
![GBuffer_CCA](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_CCA.png)
![GBuffer_MotionVector](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_MotionVector.png)
![GBuffer_Depth](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Depth.png)

#### Shadowmap绘制
类似Opaque和Transparent对象的绘制，把通过Light剪裁出来的对象准备到buffer中，针对不同的Cascade等级，绘制到不同的CascadeLevel中，
![DirectionCascadeShadowmap](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/DirectionCascadeShadowmap.png)

### GPU Driven Terrain
GPU Driven Terrain的流程
1. 使用相机Frustum剪裁场景地形
2. 使用上一帧MaxDepthPyramid重映射到当前帧剪裁剩余地形图块，并绘制地形到GBuffer
3. 重新生成depth的mipmap，MaxDpethPyramid，
4. 使用新生成的MaxDpethPyramid对剩余的地形图块进行剪裁，并绘制地形到GBuffer，这里所有的地形图块都绘制完成了
5. 重新用新的Depth生成新的MaxDepthPyramid

TODO：补充地形图块的设计和地形在GPU上的生成流程


## AO
当前实现了SSAO和HBAO

### SSAO

### HBAO

![HBAO](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/HBAO.png)


## AA

### FXAA


### TAA

![SkyBox](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/SkyBox.png)
![TemporalReprojection](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/TemporalReprojection.png)

## Bloom

![Bloom](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/Bloom.png)


## ToneMapping

![ToneMapping](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/ToneMapping.png)


















