using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Baidu.Guoke.Data.RTree;
using Baidu.Guoke.Data.Util;

namespace Baidu.Guoke.Data.Base
{
    public sealed class SimplePolygonGeometry : IEnumerable<PointGeometry>
    {
        public SimplePolygonGeometry(IEnumerable<PointGeometry> p_point)
        {
            List<PointGeometry> temp = new List<PointGeometry>(p_point);
            points = new PointGeometry[temp.Count()];
            temp.CopyTo(points);
        }

        private PointGeometry[] points;

        public int Count()
        {
            return points.Count();
        }

        public PointGeometry this[int i]
        {
            get
            {
                return points[i];
            }
        }

        public IEnumerator<PointGeometry> GetEnumerator()
        {
            return ((IEnumerable<PointGeometry>)points).GetEnumerator();
        }

        System.Collections.IEnumerator System.Collections.IEnumerable.GetEnumerator()
        {
            return points.GetEnumerator();
        }

        public bool IntersectsWith(int left, int bottom, int right, int top)
        {
            SimplePolygonGeometry geometry = this;

            for (int i = 0; i < geometry.Count(); ++i)
            {
                if (left < geometry[i].X && geometry[i].X < right && bottom < geometry[i].Y & geometry[i].Y < top)
                {
                    return true;
                }
            }

            for (int i = 1; i < geometry.Count(); ++i)
            {
                if (Common.LineIntersectRect(geometry[i - 1].X, geometry[i - 1].Y, geometry[i].X, geometry[i].Y, left, bottom, right, top))
                {
                    return true;
                }
            }

            if (Common.LineIntersectRect(geometry[0].X, geometry[0].Y, geometry[geometry.Count() - 1].X, geometry[geometry.Count() - 1].Y, left, bottom, right, top))
            {
                return true;
            }

            return false;
        }

        public Rectangle BoundRect()
        {
            SimplePolygonGeometry geometry = this;

            int xmin = geometry[0].X;
            int ymin = geometry[0].Y;
            int xmax = geometry[0].X;
            int ymax = geometry[0].Y;
            for (int i = 0; i < geometry.Count(); ++i)
            {
                if (geometry[i].X < xmin)
                {
                    xmin = geometry[i].X;
                }
                if (geometry[i].X > xmax)
                {
                    xmax = geometry[i].X;
                }
                if (geometry[i].Y < ymin)
                {
                    ymin = geometry[i].Y;
                }
                if (geometry[i].Y > ymax)
                {
                    ymax = geometry[i].Y;
                }
            }
            return new Rectangle(xmin, ymin, xmax, ymax);
        }

        public double MinDistance(int px, int py)
        {
            SimplePolygonGeometry geometry = this;
            int retmod;
            double retdis = Common.SegementDistance(px, py, geometry[0].X, geometry[0].Y, geometry[1].X, geometry[1].Y, out retmod); ;
            for (int i = 1; i < geometry.Count() - 1; ++i)
            {
                int mod;
                double dis = Common.SegementDistance(px, py, geometry[i].X, geometry[i].Y, geometry[i + 1].X, geometry[i + 1].Y, out mod);
                if (dis < retdis)
                {
                    retdis = dis;
                    retmod = mod;
                }
            }

            return retdis;
        }

        public bool IntersectsWith(LineStringGeometry linestring)
        {
            for (int i = 0; i < linestring.Count() - 1; ++i)
            {
                PointGeometry start_point = linestring[i];
                PointGeometry end_point = linestring[i + 1];

                if (IntersectWithLine(start_point, end_point))
                    return true;
            }

            return false;
        }

        private bool IntersectWithLine(PointGeometry start_point, PointGeometry end_point)
        {
            //外接矩形检测
            var MER = BoundRect();
            int min_x = Math.Min(start_point.X, end_point.X);
            int min_y = Math.Min(start_point.Y, end_point.Y);

            int max_x = Math.Max(start_point.X, end_point.X);
            int max_y = Math.Max(start_point.Y, end_point.Y);
            if (max_x < MER.min_x || max_y < MER.min_y || min_x > MER.max_x || min_y > MER.max_y)
                return false;

            //判断2点是否在多边形内或者多边形上，如果有一个在内或者上则相交
            if (IntersectWithPoint(start_point) >= 0 || IntersectWithPoint(end_point) >= 0)
                return true;

            //精细检测,检查多边形的每一条边是否与折现的线段相交
            for (int i = 0; i < points.Length; ++i)
            {
                PointGeometry s2 = points[i];
                PointGeometry e2 = points[(i + 1) % points.Length];

                bool bIntersect = SegmentIntersects(start_point, end_point, s2, e2);
                if (bIntersect)
                    return true;
            }

            return false;
        }

        //-1 该点在多边形外面
        //0 该点在多边形上
        //1 该点在多边形内
        private int IntersectWithPoint(PointGeometry start_point)
        {
            //构造一个平行与X轴的射线
            PointGeometry virtual_end_point = new PointGeometry(int.MinValue, start_point.Y);

            int cross_count = 0;
            for (int i = 0; i < points.Length; ++i)
            {
                PointGeometry side_s = points[i];
                PointGeometry side_e = points[(i + 1) % points.Length];

                //检查该点是否在边上,如果在则return 0
                if (IsOnLine(side_s, side_e, start_point))
                    return 0;

                //如果平行与X轴，则不用考虑
                if (side_s.Y == side_e.Y)
                    continue;

                if (IsOnLine(start_point, virtual_end_point, side_s))
                {
                    if (side_s.Y > side_e.Y)
                        ++cross_count;
                }
                else if (IsOnLine(start_point, virtual_end_point, side_e))
                {
                    if (side_e.Y > side_s.Y)
                        ++cross_count;
                }
                else if (SegmentIntersects(start_point, virtual_end_point, side_s, side_e))
                    ++cross_count;
            }

            return cross_count % 2 == 1 ? 1 : -1;
        }

        private bool IsOnLine(PointGeometry side_s, PointGeometry side_e, PointGeometry point)
        {
            return CrossProduct(point, side_s, side_e) == 0 &&
                point.X >= Math.Min(side_s.X, side_e.X) &&
                point.X <= Math.Max(side_s.X, side_e.X) &&
                point.Y >= Math.Min(side_s.Y, side_e.Y) &&
                point.Y <= Math.Max(side_s.Y, side_e.Y);
        }

        private static bool SegmentIntersects(PointGeometry s1, PointGeometry e1, PointGeometry s2, PointGeometry e2)
        {
            long cp1 = CrossProduct(s1, e1, s2);
            long cp2 = CrossProduct(s1, e1, e2);
            long cp3 = CrossProduct(s2, e2, s1);
            long cp4 = CrossProduct(s2, e2, e1);

            if (((cp1 > 0 && cp2 < 0)||(cp1 < 0 && cp2 > 0)) && ((cp3 > 0 && cp4 < 0)||(cp3 < 0 && cp4 > 0)))
                return true;

            if (cp1 == 0 && OnSegement(s1, e1, s2))
                return true;

            if (cp2 == 0 && OnSegement(s1, e1, e2))
                return true;

            if (cp3 == 0 && OnSegement(s2, e2, s1))
                return true;

            if (cp4 == 0 && OnSegement(s2, e2, e1))
                return true;

            return false;
        }

        //p12 × p13
        private static long CrossProduct(PointGeometry p1, PointGeometry p2, PointGeometry p3)
        {
            return ((long)p2.X - (long)p1.X) * ((long)p3.Y - (long)p1.Y) - ((long)p2.Y - p1.Y) * ((long)p3.X - (long)p1.X);
        }

        private static bool OnSegement(PointGeometry s1, PointGeometry e1, PointGeometry s2)
        {
            return s2.X >= Math.Min(s1.X, e1.X) && s2.X <= Math.Max(s1.X, e1.X) && s2.Y >= Math.Min(s1.Y, e1.Y) && s2.Y <= Math.Max(s1.Y, e1.Y);
        }

    }
}
