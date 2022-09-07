#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arm_neon.h>
#include <time.h>

struct timespec spec_start;
struct timespec spec_end;
    
void Square_neon(const int32_t* src, uint32_t* dest, int32_t len) {
    float sum=0;
    //float32x4_t sum_vec=vdupq_n_f32(0),left_vec,right_vec;
#if 0

    int32x4_t *l_a = src;
    int32x4_t *r_b = src;
    int32x4_t sum_vec;
    int i;
    int32_t* res = dest;
    
    //re = len % 4;
    
    for (i = 0; i < len; i += 4) 
    {
        sum_vec=vmulq_s32(*l_a,*r_b);
        vst1q_s32(res, sum_vec);
        
        res += 4;
        l_a++;
        r_b++;
        
    }

    
    for (; i < len; i++)
      *(dest + i) = (*(src + i)) * (*(src + i));
#else
    
    int32x4_t *l_a = src;
    int32x4_t *r_b = src;
    int32x4_t sum_vec;
    int i;
    int32_t* res = dest;
    int re1,re2;
    
    static int flg = 0;
    
    if(flg ==0 ){
        printf("Square_neon src addr %x\n",(unsigned int)src);
        flg = 1;
    }
    
    re1 = len / 4;
    
    if(re1 > 0){
        for (i = 0; i < re1; i++) 
        {
            sum_vec=vmulq_s32(*l_a,*r_b);
            vst1q_s32(res, sum_vec);
            
            res += 4;
            l_a++;
            r_b++;
            
        }
    }
    re2 = len % 4;
    
    if(re2 > 0){
        for (i= re1*4; i < len; i++)
            *(dest + i) = (*(src + i)) * (*(src + i));
    }

#endif
    
    return sum;
}   

void Square(const int32_t* src, uint32_t* dest, int32_t len) {
    int i;
    for (i = 0; i < len; i++)
      *(dest + i) = (*(src + i)) * (*(src + i));
  

}


void VectorAbs(const int32_t* src, int32_t* dest, int32_t len) {
    int32_t i;

    for (i = 0; i < len; i++)
      dest[i] = (src[i] >= 0 ? src[i] : -src[i]);

}

void VectorAbs_neon(const int32_t* src, int32_t* dest, int32_t len) {
    int32_t i;

    int32x4_t *l_a = src;

    int32x4_t sum_vec;
    int32_t* res = dest;
    

    
    for (i = 0; i < len; i += 4) 
    {
        sum_vec=vabsq_s32(*l_a);
        vst1q_s32(res, sum_vec);
        
        res += 4;
        l_a++;

        
    }

    
    for (; i < len; i++)
      dest[i] = (src[i] >= 0 ? src[i] : -src[i]);

    

}

void VectorAdd_neon(const int32_t* x, const int32_t* y, int32_t* z,int32_t len) 
{
    int32_t i;
    int32x4_t *l_a = x;
    int32x4_t *r_b = y;
    int32x4_t sum_vec;
    int32x4_t sum_vec1;

    int32_t* res = z;
    
    for (i = 0; i < len; i += 4) 
    {
        sum_vec=vaddq_s32(*l_a,*r_b);
        l_a++;
        r_b++;       
        //sum_vec1=vaddq_s32(*l_a,*r_b);
        
        
        vst1q_s32(res, sum_vec);
        //res += 4;
        
        //vst1q_s32(res, sum_vec1);
        //l_a++;
        //r_b++;
        res += 4;
        
    }

    for (; i < len; i++)
      *(z + i) = (*(x + i)) + (*(y + i));        

       
}

void VectorAdd(const int32_t* x, const int32_t* y, int32_t* z,int32_t len) 
{
    int i;
    
    for (i = 0; i < len; i++)
      *(z + i) = (*(x + i)) + (*(y + i));
    
}
  

// float dot_neon(float* A,float* B,int len) __attribute__((naked));
  
float dot_neon(float* A,float* B,int len)
{
#if 0
    float sum;
    volatile float32x4_t *l_a ;
    volatile float32x4_t *r_b ;
    float32x4_t sum_vec,sum_vec2,sum_vec3,sum_vec4;

    int i,re1,re2;
    
    sum=0;
    l_a = A;
    r_b = B;
    sum_vec = vdupq_n_f32(0);
	sum_vec2 = vdupq_n_f32(0);
	sum_vec3 = vdupq_n_f32(0);
	sum_vec4 = vdupq_n_f32(0);


   for (i = 0; i < len; i+=16) 
   {
	   
asm("	pld [%0,#32] ": : "r" (l_a));
asm("	pld [%0,#32] ": : "r" (r_b));
       sum_vec=vmlaq_f32(sum_vec,*l_a,*r_b);
       l_a++;
       r_b++;
	   
	   
       sum_vec2=vmlaq_f32(sum_vec2,*l_a,*r_b);
       l_a++;
       r_b++;       
	   sum_vec3=vmlaq_f32(sum_vec3,*l_a,*r_b);
       l_a++;
       r_b++;       
	   sum_vec4=vmlaq_f32(sum_vec4,*l_a,*r_b);
       l_a++;
       r_b++; 
		   
       
   }
   sum_vec = vaddq_f32(sum_vec,sum_vec2);
   sum_vec3 = vaddq_f32(sum_vec3,sum_vec4);
   sum_vec = vaddq_f32(sum_vec,sum_vec3);
   sum = vgetq_lane_f32(sum_vec, 0) + vgetq_lane_f32(sum_vec, 1) + vgetq_lane_f32(sum_vec, 2) + vgetq_lane_f32(sum_vec, 3);


   return sum;
#else
	
//use only on Cortex-A7
 asm("	vmov.i32	q8, #0	;"
"	cmp	r2, #0 ;"
"	vorr	q10, q8, q8 ;"
"	vorr	q9, q8, q8 ;"
"	vorr	q0, q8, q8 ;"
"	ble	1f  ;"
"	mov	r3, #0 ;"
"2:	pld [r0,#32] ;"
"	pld [r1,#32] ;"
"	vld1.64	{d4-d5}, [r0 :64] ;"
"	vld1.64	{d2-d3}, [r1 :64] ;"
"	vldr	d30, [r0, #16] ;"
"	vldr	d31, [r0, #24] ;"
"	vldr	d6, [r1, #16] ;"
"	vldr	d7, [r1, #24] ;"
"	vldr	d26, [r0, #32] ;"
"	vldr	d27, [r0, #40]	 ;"
"	vldr	d28, [r1, #32] ;"
"	vldr	d29, [r1, #40]	 ;"
"	vldr	d22, [r0, #48]	 ;"
"	vldr	d23, [r0, #56]	 ;"
"	vldr	d24, [r1, #48]	 ;"
"	vldr	d25, [r1, #56]	 ;"
"	add	r3, r3, #16 ;"
"	add	r0, r0, #64	 ;"
"	cmp	r2, r3 ;"
"	add	r1, r1, #64	 ;"
"	vmla.f32	q8, q2, q1 ;"
"	vmla.f32	q10, q15, q3 ;"
"	vmla.f32	q9, q13, q14 ;"
"	vmla.f32	q0, q11, q12 ;"
"	bgt	2b ;"
"1:	vadd.f32	q8, q8, q10 ;"
"	vadd.f32	q0, q9, q0 ;"
"	vadd.f32	q0, q8, q0 ;"
"	vmov.32	r3, d0[0] ;"
"	vmov	s14, r3 ;"
"	vmov.32	r3, d0[1] ;"
"	vmov	s15, r3 ;"
"	vmov.32	r3, d1[0] ;"
"	vadd.f32	s15, s14, s15 ;"
"	vmov	s13, r3 ;"
"	vmov.32	r3, d1[1] ;"
"	vadd.f32	s15, s15, s13 ;"
"	vmov	s0, r3 ;"
"	vadd.f32	s0, s15, s0 ;"
"	bx	lr ;");    

#endif
    
}


int dot_int_neon(int* A,int* B,int len)
{

    int sum;
    volatile int32x4_t *l_a ;
    volatile int32x4_t *r_b ;
    int32x4_t sum_vec;

    int i,re1,re2;
    
    sum=0;
    l_a = A;
    r_b = B;
    sum_vec = vdupq_n_s32(0);


   for (i = 0; i < len; i+=4) 
   {
       sum_vec=vmlaq_s32(sum_vec,*l_a,*r_b);
       l_a++;
       r_b++;
       
       
   }
   sum = vgetq_lane_s32(sum_vec, 0) + vgetq_lane_s32(sum_vec, 1) + vgetq_lane_s32(sum_vec, 2) + vgetq_lane_s32(sum_vec, 3);


   return sum;
}  
   
   
float dot(float* A,float* B,int K)
{
    float sum=0;
    int i;
    
    for(i=0;i<K;i++){
        sum += A[i]*B[i];
    }

    return sum;
}

int dot_int(int* A,int* B,int K)
{
    int sum=0;
    int i;
    
    for(i=0;i<K;i++){
        sum += A[i]*B[i];
    }

    return sum;
}


#define BUF_LEN (10*1024*1024)
#define KERNEL_LEN (10*1024*1024)

#define VECTOR_LEN (10*1024*1024)
int main()
{
    float *src, *org,*kernel, res1, res2, sum;
    int cnt,i;
    unsigned int *src1, *src2, *add_res1,*add_res2;


#if 1
    
    src = malloc(sizeof(float)*BUF_LEN+ 4096);
	src = (float *)( (unsigned int)( (unsigned int)src & 0xffffff80)+8) ;
	
	printf("src 0x%x\n",(unsigned int)src);
    //res1 = malloc(sizeof(float)*BUF_LEN/KERNEL_LEN);
    //res2 = malloc(sizeof(float)*BUF_LEN/KERNEL_LEN);
    
    for(i=0;i<BUF_LEN;i++){
        src[i] = (unsigned char)i;
        
    }
    
    kernel = malloc(sizeof(float)*KERNEL_LEN+ 4096);
	
	kernel = (float *)( (unsigned int)( (unsigned int)kernel & 0xffffff80)+8) ;
	printf("kernel 0x%x\n",(unsigned int)kernel);
    for(i=0;i<KERNEL_LEN;i++)
        kernel[i] = (unsigned char)i;   
    




// test dot Product
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    i = 0;
    printf("neon start\n");
    for(cnt=0;cnt<100;cnt++)
        res1  = dot_neon(src,kernel,KERNEL_LEN);

    printf("neon end\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("neon time %f\n",sum);
 
 
    clock_gettime(CLOCK_MONOTONIC, &spec_start); 
    i = 0;
    printf("neon start\n");
    for(cnt=0;cnt<100;cnt++)
        res2 = dot(src,kernel,KERNEL_LEN);

    printf("neon end\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("no neon time %f\n",sum);

    //for(i=0;i<BUF_LEN;i++){
        if((int)res1 != (int)res2){
            printf("err in %d, %d:%d\n",i,(int)res1,(int)res2);
           // break;
        }
    //}
#endif 


	printf("\ntest int dot prodect\n");
	
	
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    i = 0;
    printf("neon start\n");
    for(cnt=0;cnt<100;cnt++)
        res1  = dot_int_neon((int *)src,(int *)kernel,KERNEL_LEN);

    printf("neon end\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("neon time %f\n",sum);
 
 
    clock_gettime(CLOCK_MONOTONIC, &spec_start); 
    i = 0;
    printf("neon start\n");
    for(cnt=0;cnt<100;cnt++)
        res2 = dot_int((int *)src,(int *)kernel,KERNEL_LEN);

    printf("neon end\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("no neon time %f\n",sum);

    //for(i=0;i<BUF_LEN;i++){
        if((int)res1 != (int)res2){
            printf("err in %d, %d:%d\n",i,(int)res1,(int)res2);
           // break;
        }
    //}	
	
	
	
	
	
	
	
    src1 = malloc(sizeof(int)*VECTOR_LEN);
    src2 = malloc(sizeof(int)*VECTOR_LEN);
    add_res1 = malloc(sizeof(int)*VECTOR_LEN);
    add_res2 = malloc(sizeof(int)*VECTOR_LEN);
    
    
    for(i=0;i<VECTOR_LEN;i++){
        src1[i] = (unsigned char)i;
        src2[i] = (src1[i] << 8);
        
        add_res1[i] = 0xff;
        add_res2[i] = 0xff;
        
    }  
    

#if 1
    //VectorAdd_neon(src1,src2,add_res1,VECTOR_LEN);
    //VectorAdd(src1,src2,add_res2,VECTOR_LEN);
    //for(i=0;i<VECTOR_LEN;i++){
    //    if(add_res1[i] != add_res2[i])
    //        printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    //}
    printf("\r\n\r\ntest vector add\r\n");
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        VectorAdd_neon(src1,src2,add_res1,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("neon time %f\n",sum);
 
 
    clock_gettime(CLOCK_MONOTONIC, &spec_start); 
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        VectorAdd(src1,src2,add_res2,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("no neon time %f\n",sum);

    for(i=0;i<VECTOR_LEN;i++){
        if(add_res1[i] != add_res2[i])
            printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    }    
#endif


#if 1
    printf("\r\n\r\ntest vector square\r\n");
    
    //Square_neon(src1,add_res1,VECTOR_LEN);
    //Square(src1,add_res2,VECTOR_LEN);
    //for(i=0;i<VECTOR_LEN;i++){
    //    if(add_res1[i] != add_res2[i])
    //        printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    //}
    
    
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        Square_neon(src1,add_res1,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("neon time %f\n",sum);
 
 
    clock_gettime(CLOCK_MONOTONIC, &spec_start); 
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        Square(src1,add_res2,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("no neon time %f\n",sum);

    for(i=0;i<VECTOR_LEN;i++){
        if(add_res1[i] != add_res2[i])
            printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    }        
#endif      

#if 1
    printf("\r\n\r\ntest vector abs\r\n");
    for(i=0;i<VECTOR_LEN;i++){
        src1[i] = -256 + (unsigned char)i;
    }     
    //VectorAbs_neon(src1,add_res1,513);
    //VectorAbs(src1,add_res2,513);
    //for(i=0;i<513;i++){
    //    if(add_res1[i] != add_res2[i])
    //        printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    //}
    
    
    clock_gettime(CLOCK_MONOTONIC, &spec_start);
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        VectorAbs_neon(src1,add_res1,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("neon time %f\n",sum);
 
 
    clock_gettime(CLOCK_MONOTONIC, &spec_start); 
    i = 0;
    for(cnt=0;cnt<100;cnt++)
        VectorAbs(src1,add_res2,VECTOR_LEN);
    
    clock_gettime(CLOCK_MONOTONIC, &spec_end);
    sum = ( spec_end.tv_sec - spec_start.tv_sec + (spec_end.tv_nsec - spec_start.tv_nsec)/1000000000.0 );
    printf("no neon time %f\n",sum);

    for(i=0;i<VECTOR_LEN;i++){
        if(add_res1[i] != add_res2[i])
            printf("err in %d, %d:%d\n",i,add_res1[i],add_res2[i]);
    }      
#endif
 
    return 0;
}