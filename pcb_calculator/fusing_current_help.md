You can use the Onderdonk equation to check if a small track can handle a large current for a short period of time.
This tool allows you to design a track fuse but should be used as an estimate only.

<center>__8.9 * 10 <sup>-6</sup> * t * ( I / A ) <sup>2</sup>  = log<sub>10</sub> ( (T<sub>m</sub> - T<sub>a</sub> ) / ( 234 + T<sub>a</sub>  ) +1 )__</center>

where: 

__I__ = current in A  
__T<sub>m</sub>__ = Metal melting point in degree C  
__T<sub>a</sub>__ = Ambient temperature in degree C  
__A__ = Cross section area in mm<sup>2</sup>  
__t__ = Time to fuse in s



A real track __always__ melts later than the prediction with this equation because:

- The equation assumes no heat dissipation at all.

- The equation predicts the initial onset of melting