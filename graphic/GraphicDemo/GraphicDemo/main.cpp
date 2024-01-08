#include <vector>
#include <iostream>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include "ourgl.h"

// 扫线算法，bresenham
/*void drawLine(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor& color)
{
	// 1、因为这样会导致当高更大时，导致点不够，直线会断断续续
	// 2、有除法会导致被除数为零
	int x = 0, y = 0;
	for (x = x0; x <= x1; x++)
	{
		auto t = (x - x0) / (float)(x1 - x0);
		y = y0*(1 - t) + y1*t;
		image.set(x, y, color);
	}

	// 保证了斜率小于等于45度，error第一次不可能大于1
	int x = 0, y = 0;
	bool steep = false;
	if (std::abs(x1 - x0) < std::abs(y1 - y0))
	{
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1)
	{
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int dy = y1 - y0;
	int dx = x1 - x0;
	int derror = std::abs(dy) * 2;
	int error = 0;

	y = y0;
	for (x = x0; x <= x1; x++)
	{
		if (steep)
			image.set(y, x, color);
		else
			image.set(x, y, color);

		error += derror;
		if (error > dx)
		{
			y += (y1 > y0) ? 1 : -1;
			error -= dx * 2;
		}
	}
}*/

/*Vec3f m2v(Matrix m) {
	return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}*/

/*Matrix v2m(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.f;
	return m;
}*/

/*Matrix viewport(int x, int y, int w, int h) {
	Matrix m = Matrix::identity(4);
	m[0][3] = x + w / 2.f;
	m[1][3] = y + h / 2.f;
	m[2][3] = depth / 2.f;

	m[0][0] = w / 2.f;
	m[1][1] = h / 2.f;
	m[2][2] = depth / 2.f;
	return m;
}*/

// 这是基于cpu的特性而设计的扫线（scanline）算法，但是对于gpu来说不能并行的计算，因此需要换一种方法S
/*void drawTriangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color)
{
	// 扫线算法，分两段绘制，先求出离散点再绘制三角形
	if (t0.y == t1.y && t0.y == t2.y) return;
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);

	bool second_half = false; // 判断绘制的是下半部分还是上半部分
	int total_heigh = t2.y - t0.y;
	int segment_heigh = 0;
	for (int y = t0.y; y <= t2.y; y++)
	{
		if (y > t1.y)
			second_half = true;

		segment_heigh = second_half ? t2.y - t1.y + 1 : t1.y - t0.y + 1;
		float alpha = (float)(y - t0.y) / total_heigh;
		float beta = (second_half ? (float)(y - t1.y) : (float)(y - t0.y)) / segment_heigh;

		Vec2i A = t0 + (t2 - t0) * alpha;
		Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;

		if (A.x > B.x) std::swap(A, B);
		for (int x = A.x; x <= B.x; x++)
			image.set(x, y, color);
	}
}*/

// 基于重心坐标求出点P是否在三角形中，从而绘制三角形
/*Vec3f barycentric(Vec3f* pts, Vec3f p)
{
	// 通过叉乘求得重心坐标
	Vec3f u = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x) ^ Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - p.y);
	if (std::abs(u.z) < 1e-2) return Vec3f(-1, 1, 1);// 按理来说大于0就行，但是考虑计算的误差，小于1普遍认为这个点不行
	return Vec3f(1.f - (u.x + u.y) / u.z, u.x / u.z, u.y / u.z);// 返回1-u-v，u，v
}

void triangle(Vec3f* pts, float* zbuffer, TGAImage& image, TGAColor color)
{
	Vec2f boxMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f boxMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

	// 获取包围盒，提升判断的效率
	for (int i = 0; i < 3; i++)
	{
		boxMin.x = std::max(0.f, std::min(boxMin.x, pts[i].x));
		boxMin.y = std::max(0.f, std::min(boxMin.y, pts[i].y));

		boxMax.x = std::min((float)(image.get_width() - 1), std::max(boxMax.x, pts[i].x));
		boxMax.y = std::min((float)(image.get_height() - 1), std::max(boxMax.y, pts[i].y));
	}
	Vec3f point;
	for(point.x = boxMin.x; point.x <= boxMax.x; point.x++)
		for (point.y = boxMin.y; point.y <= boxMax.y; point.y++)
		{
			auto barycentricPoint = barycentric(pts, point);
			if (barycentricPoint.x < 0 || barycentricPoint.y < 0 || barycentricPoint.z < 0)
				continue;// 用来判断当前像素是否在三角形中，重心坐标权重只要有一个参数小于0则不在三角形中
			point.z = 0;
			for (int i = 0; i < 3; i++)
				point.z += pts[i].z * barycentricPoint[i];
			if (zbuffer[int(point.x + point.y * width)] < point.z)
			{
				zbuffer[int(point.x + point.y * width)] = point.z;
				image.set(point.x, point.y, color);
			}
		}
}*/

// 加上了纹理的三角形绘制
/*void triangle(Vec3i t0, Vec3i t1, Vec3i t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage& image, float intensity, int* zbuffer) {
	if (t0.y == t1.y && t0.y == t2.y) return; // i dont care about degenerate triangles
	if (t0.y > t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
	if (t0.y > t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
	if (t1.y > t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

	int total_height = t2.y - t0.y;
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1.y - t0.y || t1.y == t0.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1.y - t0.y : 0)) / segment_height; // be careful: with above conditions no division by zero here
		Vec3i A = t0 + Vec3f(t2 - t0) * alpha;
		Vec3i B = second_half ? t1 + Vec3f(t2 - t1) * beta : t0 + Vec3f(t1 - t0) * beta;
		Vec2i uvA = uv0 + (uv2 - uv0) * alpha;
		Vec2i uvB = second_half ? uv1 + (uv2 - uv1) * beta : uv0 + (uv1 - uv0) * beta;
		if (A.x > B.x) { std::swap(A, B); std::swap(uvA, uvB); }
		for (int j = A.x; j <= B.x; j++) {
			float phi = B.x == A.x ? 1. : (float)(j - A.x) / (float)(B.x - A.x);
			Vec3i   P = Vec3f(A) + Vec3f(B - A) * phi;
			Vec2i uvP = uvA + (uvB - uvA) * phi;
			int idx = P.x + P.y * width;
			if (zbuffer[idx] < P.z) {
				zbuffer[idx] = P.z;
				TGAColor color = model->diffuse(uvP);
				image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity, 255));
			}
		}
	}
}*/

/*Vec3f world2screen(Vec3f v) 
{
	return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}*/

Model* model = NULL;
const int width = 800;
const int height = 800;

Vec3f light_dir(0, 0, 1);
Vec3f       eye(0, 0, 3);
Vec3f    center(0, 0, 0);
Vec3f        up(0, 1, 0);

struct GouraudShader : public IShader {
	Vec3f varying_intensity; // written by vertex shader, read by fragment shader
	mat<2, 3, float> varying_uv;
	mat<4, 4, float> uniform_M;   //  Projection*ModelView
	mat<4, 4, float> uniform_MIT; // (Projection*ModelView).invert_transpose()

	// Gouraud阴影实现，在顶点着色器阶段计算三角形各个顶点与光源的光照强度（与光源的点积）varying_intensity，在片段着色器根据重心算法插值
	// 优点就是计算成本低，但是会失真
	virtual Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_Vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		gl_Vertex = Viewport * Projection * ModelView * gl_Vertex;     // transform it to screen coordinates
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir); // get diffuse lighting intensity
		return gl_Vertex;
	}

	virtual bool fragment(Vec3f bar, TGAColor& color) {
		/*
		//float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
		//if (intensity > .85) intensity = 1;
		//else if (intensity > .60) intensity = .80;
		//else if (intensity > .45) intensity = .60;
		//else if (intensity > .30) intensity = .45;
		//else if (intensity > .15) intensity = .30;
		//else intensity = 0;
		//color = TGAColor(255, 155, 0) * intensity; // well duh
		//return false;                              // no, we do not discard this pixel
		*/
		
		 //Gouraud阴影实现 
		/*float intensity = varying_intensity * bar;   // interpolate intensity for the current pixel
		color = TGAColor(255, 255, 255) * intensity; // well duh
		return false;*/

		// phong阴影实现，环境分量、漫反射diff、镜面反射spec
		Vec2f uv = varying_uv * bar;
		Vec3f n = proj<3>(uniform_MIT * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(uniform_M * embed<4>(light_dir)).normalize();
		Vec3f r = n * (n * l * 2) - l;
		float spec = pow(std::max(r.z, 0.0f), model->specular(uv));//镜面反射就是一坨光，幂函数参数是控制光团是否发散还是集中
		float diff = std::max(0.f, n * l);
		TGAColor c = model->diffuse(uv);
		color = c;
		for (int i = 0; i < 3; i++) color[i] = std::min<float>(5 + c[i] * (diff + .6 * spec), 255);
		return false;

		// 阴影映射方法：先将相机移动到光源处收集阴影缓存数据，再将相机移到本身的位置通过深度对比看是不是在阴影下
	}
};

int main(int argc, char** argv) 
{
	TGAImage image(width, height, TGAImage::RGB);

	lookat(eye, center, up);
	viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	projection(-1.f / (eye - center).norm());
	light_dir.normalize();

	TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

	model = new Model("E:/yt/data/opgl/datas/african_head.obj");

	GouraudShader shader;
	shader.uniform_M = Projection * ModelView;
	shader.uniform_MIT = (Projection * ModelView).invert_transpose();

	for (int i = 0; i < model->nfaces(); i++) {
		Vec4f screen_coords[3];
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, image, zbuffer);
	}

	zbuffer.flip_vertically();
	zbuffer.write_tga_file("zbuffer.tga");
	delete model;
	// 搭建基本平台
	/*drawLine(13, 20, 80, 40, image, white);
	drawLine(20, 13, 40, 80, image, red);
	drawLine(80, 40, 13, 20, image, red);
	image.flip_vertically();// 假设原点在左下角
	image.write_tga_file("output.tga");
	return 0;*/

	// 绘制直线，bresenham算法
	/*model = new Model("E:/yt/data/opgl/datas/african_head.obj");
	for (int i = 0; i < model->nfaces(); i++) 
	{
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++) 
		{
			Vec3f v0 = model->vert(face[j]);
			Vec3f v1 = model->vert(face[(j + 1) % 3]);
			int x0 = (v0.x + 1.) * width / 2.;
			int y0 = (v0.y + 1.) * height / 2.;
			int x1 = (v1.x + 1.) * width / 2.;
			int y1 = (v1.y + 1.) * height / 2.;
			drawLine(x0, y0, x1, y1, image, white);
		}
	}
	image.flip_vertically();
	image.write_tga_file("output.tga");
	delete model;*/

	// 基于cpu绘制三角形
	/*Vec2i t0[3] = { Vec2i(10, 70),   Vec2i(50, 160),  Vec2i(70, 80) };
	Vec2i t1[3] = { Vec2i(180, 50),  Vec2i(150, 1),   Vec2i(70, 180) };
	Vec2i t2[3] = { Vec2i(180, 150), Vec2i(120, 160), Vec2i(130, 180) };
	drawTriangle(t0[0], t0[1], t0[2], image, red);
	drawTriangle(t1[0], t1[1], t1[2], image, white);
	drawTriangle(t2[0], t2[1], t2[2], image, green);*/

	// 基于Gpu绘制三角形，可以支持大量并行执行
	/*Vec2i pts[3] = { Vec2i(10,10), Vec2i(100, 30), Vec2i(190, 160) };
	triangle(pts, image, red);*/

	// 随机着色，根据光照着色
	/*model = new Model("E:/yt/data/opgl/datas/african_head.obj");

	Vec2i screen_coords[3];
	Vec3f coords[3];
	for (int i = 0; i < model->nfaces(); i++)
	{
		std::vector<int> face = model->face(i);
		for (int j = 0; j < 3; j++)
		{
			coords[j] = model->vert(face[j]);
			screen_coords[j] = Vec2i((coords[j].x + 1) * width / 2, (coords[j].y + 1) * height / 2);
		}
		Vec3f n = (coords[2] - coords[0]) ^ (coords[1] - coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if(intensity > 0)
			drawTriangle(screen_coords[0], screen_coords[1], screen_coords[2], image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}
	delete model;*/

	// z-buffer缓冲区
	/*model = new Model("E:/yt/data/opgl/datas/african_head.obj");

	float* zbuffer = new float[width * height];
	for (int i = width * height; i--; zbuffer[i] = -std::numeric_limits<float>::max());

	Vec3f coords[3];
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3f pts[3];
		for (int i = 0; i < 3; i++)
		{
			coords[i] = model->vert(face[i]);
			pts[i] = world2screen(coords[i]);
		}
		Vec3f n = (coords[2] - coords[0]) ^ (coords[1] - coords[0]);// world2screen前的坐标
		n.normalize();
		float intensity = n * light_dir;
		triangle(pts, zbuffer, image, TGAColor(intensity * 255, intensity * 255, intensity * 255, 255));
	}
	delete model;
	delete [] zbuffer;*/

	// 加入视觉矩阵和裁剪矩阵
	/*model = new Model("E:/yt/data/opgl/datas/african_head.obj");
	Matrix Projection = Matrix::identity(4);
	Matrix ViewPort = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
	Projection[3][2] = -1.f / camera.z;

	auto zbuffer = new int[width * height];
	for (int i = 0; i < width * height; i++) {
		zbuffer[i] = std::numeric_limits<int>::min();
	}

	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<int> face = model->face(i);
		Vec3i screen_coords[3];
		Vec3f world_coords[3];
		for (int j = 0; j < 3; j++) {
			Vec3f v = model->vert(face[j]);
			screen_coords[j] = m2v(ViewPort * Projection * v2m(v));
			world_coords[j] = v;
		}
		Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
		n.normalize();
		float intensity = n * light_dir;
		if (intensity > 0) {
			Vec2i uv[3];
			for (int k = 0; k < 3; k++) {
				uv[k] = model->uv(i, k);
			}
			triangle(screen_coords[0], screen_coords[1], screen_coords[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
		}
	}*/

	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}