
-module(queuec). % define nombre del modulo

-export([empty/0, insert/2, remove/1]). % funciones disponibles desde fuera del modulo. empty 0 argumentos, insert 2 y remove 1

empty() -> % crea cola vacía
    {[], []}. % front y rear

insert({Front, Rear}, Elem) -> % inserta elemento en la cola, recibe dos argumentos, cola y el elemento
    {Front, [Elem | Rear]}. % devuelve nueva cola, elem al principio de rear

remove({[], []}) -> % si cola vacia devuelve empty
    empty; 
remove({[H | T], Rear}) -> % cola tiene front y rear, front tiene head y tale
    {ok, H, {T, Rear}}; % devuelve ok, el head y el resto de la cola sin ese elemento
remove({[], Rear}) -> % si front esta vacia y rear tiene elementos
    remove({lists:reverse(Rear), []}). % invierte rear, la convierte en nueva parte front, rear vacía, vuelve a llamar a remove
