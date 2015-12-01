
#include <stdio.h>

int main(void) {
	char str[]="0,3853.223424,7724.363996,146.693708,20151127051315.000,59,9,0.000000,149.375275";
	char lat[20],lon[20],time[30];
	int comma=0,j=0,i;
	
	for(i=0; str[i]!='\0';i++)
	{
	    if(str[i]==',')
	        {
	            comma++;
	            i++;
	            j=0;
	        }
	        
	    if(comma<1)
	        continue;
	        
	    if(comma==1)
	    {
	        if(j==2)
	        {
	            lat[j]=' ';
	            j++;
	        }
	        lat[j]=str[i];
	        j++;
	        lat[j]=' ';
	        lat[j+1]='N';
	        lat[j+2]='\0';
	        
	    }

	    if(comma==2)
	    {   
	        if(j==2)
	        {
	            lon[j]=' ';
	            j++;
	        }
	        lon[j]=str[i];
	        j++;
	        lon[j]=' ';
	        lon[j+1]='W';
	        lon[j+2]='\0';
	    }
	    
	    if(comma==4)
	    {
	        if(j==4 || j==7)
	        {
                time[j]='-';
                j++;
            }
            
            if(j==10)
            {
                time[j]=' ';
                j++;
            }
            
            if(j==13 || j==16)
            {
                time[j]=':';
                j++;
            }
            
	        time[j]=str[i];
	        j++;
	        time[j]='\0';
	   }
	    
	    if(comma>4)
	        break;
	}
	printf("Lat: %s \n",lat);
	printf("Lon: %s \n",lon);
	printf("Time: %s \n",time);
	return 0;
}

