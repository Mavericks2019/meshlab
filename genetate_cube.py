import numpy as np

def generate_cube_with_subdivisions(subdivisions=8):
    """
    生成细分后的立方体网格
    
    参数:
    subdivisions: 每个边的细分次数，控制顶点数量
    返回: vertices(顶点), faces(面)
    """
    vertices = []
    faces = []
    
    # 立方体的8个原始顶点
    base_vertices = np.array([
        [-0.5, -0.5, -0.5],  # 0: 左-下-后
        [ 0.5, -0.5, -0.5],  # 1: 右-下-后
        [ 0.5,  0.5, -0.5],  # 2: 右-上-后
        [-0.5,  0.5, -0.5],  # 3: 左-上-后
        [-0.5, -0.5,  0.5],  # 4: 左-下-前
        [ 0.5, -0.5,  0.5],  # 5: 右-下-前
        [ 0.5,  0.5,  0.5],  # 6: 右-上-前
        [-0.5,  0.5,  0.5]   # 7: 左-上-前
    ])
    
    # 细分参数 - 调整以获得约600个顶点
    if subdivisions < 1:
        subdivisions = 1
    
    # 生成6个面的细分网格
    face_offsets = [
        # 前后面 (Z方向)
        ([0, 1, 5, 4], [0, 0, -1]),  # 后面
        ([2, 3, 7, 6], [0, 0, 1]),   # 前面
        
        # 左右面 (X方向)
        ([3, 0, 4, 7], [-1, 0, 0]),  # 左面
        ([1, 2, 6, 5], [1, 0, 0]),   # 右面
        
        # 上下面 (Y方向)
        ([3, 2, 1, 0], [0, -1, 0]),  # 下面
        ([4, 5, 6, 7], [0, 1, 0])    # 上面
    ]
    
    vertex_offset = 0
    
    for corner_indices, normal in face_offsets:
        # 获取面的四个角点
        corners = [base_vertices[i] for i in corner_indices]
        
        # 在面上生成细分网格
        for i in range(subdivisions + 1):
            for j in range(subdivisions + 1):
                # 计算参数化坐标
                u = i / subdivisions
                v = j / subdivisions
                
                # 双线性插值得到顶点位置
                # 使用双线性插值公式: (1-u)(1-v)*P00 + u(1-v)*P10 + (1-u)v*P01 + uv*P11
                p00, p10, p01, p11 = corners
                
                # 使用更精确的双线性插值
                vertex = (1-u)*(1-v)*p00 + u*(1-v)*p10 + (1-u)*v*p01 + u*v*p11
                
                # 添加微小偏移避免重合顶点
                vertex += np.array(normal) * 0.0001
                
                vertices.append(vertex)
        
        # 生成该面的三角形
        for i in range(subdivisions):
            for j in range(subdivisions):
                # 计算当前网格单元的四个顶点索引
                idx0 = vertex_offset + i * (subdivisions + 1) + j
                idx1 = idx0 + 1
                idx2 = vertex_offset + (i + 1) * (subdivisions + 1) + j
                idx3 = idx2 + 1
                
                # 创建两个三角形组成一个四边形
                faces.append([idx0, idx1, idx2])
                faces.append([idx1, idx3, idx2])
        
        vertex_offset += (subdivisions + 1) * (subdivisions + 1)
    
    return np.array(vertices), np.array(faces)

def generate_cube_optimized_for_600_points():
    """
    生成接近600个点的立方体网格
    """
    vertices = []
    faces = []
    
    # 计算细分级别，使总顶点数接近600
    # 每个面有 (n+1)*(n+1) 个顶点，6个面共有 6*(n+1)^2 个顶点
    # 但实际会有重复，这里简化计算
    n = int((600 / 6) ** 0.5) - 1
    n = max(n, 3)  # 至少3x3细分
    print(f"使用细分级别: {n}x{n}, 大约 {6*(n+1)*(n+1)} 个顶点")
    
    # 立方体的8个角点
    base_vertices = np.array([
        [-0.5, -0.5, -0.5],
        [ 0.5, -0.5, -0.5],
        [ 0.5,  0.5, -0.5],
        [-0.5,  0.5, -0.5],
        [-0.5, -0.5,  0.5],
        [ 0.5, -0.5,  0.5],
        [ 0.5,  0.5,  0.5],
        [-0.5,  0.5,  0.5]
    ])
    
    # 6个面的方向
    faces_dirs = [
        ("back", [0, 1, 2, 3], [0, 0, -0.5]),    # 后面
        ("front", [4, 5, 6, 7], [0, 0, 0.5]),    # 前面
        ("left", [0, 3, 7, 4], [-0.5, 0, 0]),    # 左面
        ("right", [1, 5, 6, 2], [0.5, 0, 0]),    # 右面
        ("bottom", [0, 1, 5, 4], [0, -0.5, 0]),  # 底面
        ("top", [3, 2, 6, 7], [0, 0.5, 0])       # 顶面
    ]
    
    vertex_count = 0
    face_count = 0
    
    for face_name, indices, center_offset in faces_dirs:
        # 获取面的四个角点
        corners = [base_vertices[i] for i in indices]
        
        # 在面上生成网格点
        for i in range(n + 1):
            for j in range(n + 1):
                # 参数化坐标
                u = i / n
                v = j / n
                
                # 计算面上的点位置
                # 简单方法：使用重心坐标插值
                if i == 0 and j == 0:
                    point = corners[0]
                elif i == n and j == 0:
                    point = corners[1]
                elif i == 0 and j == n:
                    point = corners[3]
                elif i == n and j == n:
                    point = corners[2]
                else:
                    # 四边形插值
                    p1 = corners[0] * (1-u) * (1-v)
                    p2 = corners[1] * u * (1-v)
                    p3 = corners[2] * u * v
                    p4 = corners[3] * (1-u) * v
                    point = p1 + p2 + p3 + p4
                
                vertices.append(point)
        
        # 生成三角形
        for i in range(n):
            for j in range(n):
                # 计算四个顶点在当前面中的索引
                v0 = vertex_count + i * (n + 1) + j
                v1 = v0 + 1
                v2 = vertex_count + (i + 1) * (n + 1) + j
                v3 = v2 + 1
                
                # 创建两个三角形（确保法线方向一致）
                faces.append([v0 + 1, v1 + 1, v2 + 1])  # OBJ索引从1开始
                faces.append([v1 + 1, v3 + 1, v2 + 1])
                face_count += 2
        
        vertex_count += (n + 1) * (n + 1)
    
    vertices = np.array(vertices)
    
    print(f"生成了 {len(vertices)} 个顶点, {len(faces)} 个面")
    return vertices, faces

def save_obj(filename, vertices, faces):
    """
    保存为OBJ文件
    
    参数:
    filename: 输出文件名
    vertices: 顶点数组
    faces: 面数组
    """
    with open(filename, 'w') as f:
        # 写入头部信息
        f.write(f"# OBJ文件 - 三角形网格方块\n")
        f.write(f"# 顶点数: {len(vertices)}\n")
        f.write(f"# 面数: {len(faces)}\n\n")
        
        # 写入顶点
        f.write("# 顶点坐标 (x, y, z)\n")
        for i, v in enumerate(vertices):
            f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
        
        f.write("\n# 纹理坐标 (可选)\n")
        f.write("# vt 0.0 0.0\n")
        
        f.write("\n# 法线 (可选)\n")
        f.write("# vn 0.0 0.0 1.0\n")
        
        # 写入面
        f.write("\n# 三角形面 (使用顶点索引)\n")
        for face in faces:
            # OBJ格式中顶点索引从1开始
            f.write(f"f {face[0]} {face[1]} {face[2]}\n")
    
    print(f"OBJ文件已保存: {filename}")

def main():
    # 生成接近600个点的立方体网格
    print("生成三角形网格方块...")
    vertices, faces = generate_cube_optimized_for_600_points()
    
    # 保存为OBJ文件
    filename = "cube_tri_mesh.obj"
    save_obj(filename, vertices, faces)
    
    # 显示统计信息
    print(f"\n网格统计:")
    print(f"顶点数: {len(vertices)}")
    print(f"三角形面数: {len(faces)}")
    print(f"文件已保存为: {filename}")
    
    # 验证顶点数量
    if abs(len(vertices) - 600) <= 50:
        print(f"✓ 顶点数量接近600个 (实际: {len(vertices)})")
    else:
        print(f"⚠ 顶点数量为 {len(vertices)}，与600有差距")
        
        # 如果需要更接近600，可以尝试调整细分级别
        print("\n可选: 尝试不同的细分级别...")
        for n in range(5, 15):
            approx_vertices = 6 * (n + 1) * (n + 1)
            print(f"细分级别 {n}x{n}: 大约 {approx_vertices} 个顶点")

if __name__ == "__main__":
    main()