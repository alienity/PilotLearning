> 练习渲染技术和学习d3d12的渲染器，基于RenderGraph自动生成Pass层级，使用全bindless，将所有的几乎全部计算都在GPU上完成。参考Pilot渲染器，只使用了其资源和UI样式。

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

### 直接根据RenderGraph生成的可视化Graph
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

<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Color.png" alt="GBuffer_Color" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Normal.png" alt="GBuffer_Normal" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Tangent.png" alt="GBuffer_Tangent" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/Material_Normal.png" alt="Material_Normal" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_MRRA.png" alt="GBuffer_MRRA" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_CCA.png" alt="GBuffer_CCA" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_MotionVector.png" alt="GBuffer_MotionVector" width="256">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/GBuffer_Depth.png" alt="GBuffer_Depth" width="256">
</div>

#### Shadowmap绘制
类似Opaque和Transparent对象的绘制，把通过Light剪裁出来的对象准备到buffer中，针对不同的Cascade等级，绘制到不同的CascadeLevel中，采样阴影时使用PCF抗锯齿
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/DirectionCascadeShadowmap.png" alt="DirectionCascadeShadowmap" width="512">
</div>

### GPU Driven Terrain
GPU Driven Terrain的流程
1. 准备基础地形图块，允许多级MipLevel图块相邻时退化顶点，确保不同Level之间顶点之间都是无缝连接的
2. 在ComputeShader中生成多级MipLevel图块，图块包含图块坐标和缩放，以及相邻图块的miplevel
3. 使用相机Frustum剪裁场景地形图块，剔除相机视锥之外的图块
4. 使用上一帧`MaxDepthPyramid`重映射到当前帧剪裁剩余地形图块，并绘制地形到GBuffer
5. 重新生成depth的mipmap，即`MaxDpethPyramid`
6. 使用新生成的`MaxDpethPyramid`对剩余的地形图块进行剪裁，并绘制地形到GBuffer，这里所有的地形图块都绘制完成了
7. 重新用新的Depth生成新的`MaxDepthPyramid`，给后处理和下一帧剪裁时使用

#### 图块mesh结构
图块mesh中在边缘顶点都存了需要退化的值，可视化出来如下，我们将要退化的值存到mesh的Color通道上
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/mesh_terrain.png" alt="mesh_terrain" width="256">
</div>

#### 绘制时的顶点退化
在绘制与高一级level的mesh地块的时候，与高一级图块相连的图块的mesh顶点要退化到低一级mip的地块的边缘或者内里的一个点上
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/mesh_diffuse.png" alt="mesh_diffuse" width="256">
</div>

#### 绘制阴影的示例
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/shadow_terrain.png" alt="shadow_terrain" width="512">
</div>

## AO
当前实现了SSAO和HBAO

### SSAO
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/SSAO.png" alt="SSAO" width="512">
</div>

### HBAO
<div style="display:inline-block">
  <img src="https://raw.githubusercontent.com/alienity/PilotLearning/main/data/HBAO.png" alt="HBAO" width="512">
</div>


## AA

![Transparent](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/Transparent.png)

### FXAA
![Transparent](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/FXAA.png)

### TAA
![TemporalReprojection](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/TemporalReprojection.png)

## Bloom

![Bloom](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/Bloom.png)


## ToneMapping

![ToneMapping](https://raw.githubusercontent.com/alienity/PilotLearning/main/data/ToneMapping.png)


















