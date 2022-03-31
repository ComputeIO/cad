You can use the Onderdonk equation to check if a small track can handle huge current for a small amount of time.
You may use it to help you design a track fuse, but results are not accurate and require further validation.

<center>__8.9 * 10 <sup>-6</sup> * t * ( I / A ) <sup>2</sup>  = log<sub>10</sub> ( (T<sub>m</sub> - T<sub>a</sub> ) / ( 233 + T<sub>a</sub>  ) +1 )__</center>

where: 

__I__ = current in A  
__T<sub>m</sub>__ = Metal melting point in ºC  
__T<sub>a</sub>__ = Ambiant temperature in ºC  
__A__ = Cross section area in mm<sup>2</sup>  
__t__ = Time to fuse in s



A real track __always__ melts later than the prediction with this equation because:

- The equation assumes no heat dissipation at all.

- The equation does not predict when a track got molten, but predicts the onset of melting.

- The track might never melt.