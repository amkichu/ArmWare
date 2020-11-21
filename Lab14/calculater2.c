// program shows the function used to calculate r2.
unsigned long checkLinearity( long x0, long y0,
       long x1, long y1,
       long x2, long y2,
       long x3, long y3,
       long x4, long y4){
   long r2, sumx, sumy, sumxy, sumx2, sumy2, n;
   long numerator25, denominator0_25; 
   // divide by 2^n until numerator is less than sqrt(2^31-1)=46,340
   n = 0;
   sumx = (x0 + x1 + x2 + x3 + x4);
   sumy = (y0 + y1 + y2 + y3 + y4);
   sumxy = ((x0*y0)+(x1*y1)+(x2*y2)+(x3*y3)+(x4*y4)); // sum of x*y
   sumx2 = ((x0*x0)+(x1*x1)+(x2*x2)+(x3*x3)+(x4*x4)); // sum of x^2
   sumy2 = ((y0*y0)+(y1*y1)+(y2*y2)+(y3*y3)+(y4*y4)); // sum of y^2
   numerator25 = 5*sumxy - sumx*sumy;
   while(((numerator25>46340) || (numerator25<-46340)) && (n<8)){
     x0 = x0>>1; x1 = x1>>1; x2 = x2>>1; x3 = x3>>1; x4 = x4>>1;
     y0 = y0>>1; y1 = y1>>1; y2 = y2>>1; y3 = y3>>1; y4 = y4>>1;
     n = n + 1;
     sumx = (x0 + x1 + x2 + x3 + x4);
     sumy = (y0 + y1 + y2 + y3 + y4);
     sumxy = ((x0*y0)+(x1*y1)+(x2*y2)+(x3*y3)+(x4*y4)); // sum of x*y
     sumx2 = ((x0*x0)+(x1*x1)+(x2*x2)+(x3*x3)+(x4*x4)); // sum of x^2
     sumy2 = ((y0*y0)+(y1*y1)+(y2*y2)+(y3*y3)+(y4*y4)); // sum of y^2
     numerator25 = 5*sumxy - sumx*sumy;
   }
   denominator0_25 = (5*sumx2 - sumx*sumx)*(5*sumy2 - sumy*sumy)/100;
   r2 = numerator25*numerator25/denominator0_25;
   return r2;
 }