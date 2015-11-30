#include <stdio.h>

int main(void) {
	char str[]="0,3853.223424,7724.363996,146.693708,20151127051315.000,59,9,0.000000,149.375275";
	char lat[15],lon[15];
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
	        lat[j]=str[i];
	        j++;
	        lat[j]='\0';
	        
	    }

	    if(comma==2)
	    {
	        lon[j]=str[i];
	        j++;
	        lon[j]='\0';
	    }
	    
	    if(comma>2)
	        break;
	}
	printf("Lat: %s \n",lat);
	printf("Lon: %s \n",lon);
	return 0;
}

