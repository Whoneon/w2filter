# Shipped anchor datasets

Fallback inputs for `make test` on machines without nauty. Generated
with nauty 2.8.8 (`Nauty&Traces version 2.8081`):

```sh
nauty-geng -q -f -C 11 15:15 > c11_15.g6    # 947 graphs
nauty-geng -q -f -C 12 17:17 > c12_17.g6    # 6905 graphs
```

With nauty installed, `make test` regenerates the classes and these
files are not used. The (14,20) class (490047 graphs, ~7 MB) is not
shipped; that anchor is skipped without nauty.
