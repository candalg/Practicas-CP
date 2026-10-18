
-module(linec). % nombre del modulo

-export([start/1, send/2, bounce/3, stop/1]). % funciones que se pueden usar desde fuera del modulo y el numero de argumentos

start(N) when N > 0 -> % crear la linea con N procesos N mayor que 0
    setup(0, N - 1, none). % 0 el id del primer proceso y el id maximo, none porque el primero no tiene proceso anterior

setup(Id, MaxId, PrevPid) when Id == MaxId -> % si id actual igual al maximo este es ultimo proceso a crear
    spawn(fun() -> loop(Id, none, PrevPid) end); % crea ultimo proceso y lo pone a esperar mensaje 
setup(Id, MaxId, PrevPid) -> % si no es ultimo proceso crea proceso y ejecuta init loop
    spawn(fun() -> init_loop(Id, MaxId, PrevPid) end). 

init_loop(Id, MaxId, PrevPid) -> % crea siguiente proceso y empieza a esperar mensajes
    NextPid = setup(Id + 1, MaxId, self()), % este proceso crea al siguiente añadiendo 1 a id, poniendo cual es el maximo y diciendo que el que lo crea es su anterior con self
    loop(Id, NextPid, PrevPid). % entra en bucle a esperar mensaje

loop(Id, NextPid, PrevPid) -> % hace que cada proceso se quede esperando mensajes
    receive % instruccion para leer mensajes
        {send, Msg} ->  % tipo y contenido del mensaje
            io:format("~p received message ~p~n", [Id, Msg]), % muestra por pantalla el mensaje de que p id recivio el mensaje n
            if NextPid =/= none -> NextPid ! {send, Msg}; % comprueba si existe proceso siguiente y manda el mismo mensaje al siguiente proceso
               true -> ok % si no hay siguiente proceso 
            end,
            loop(Id, NextPid, PrevPid); % proceso vuelve a quedarse esperando ams mensajes
            
        {bounce, Msg, Times, Dir} -> % tipo de mensaje, contenido, pasos y direccion actual
            io:format("~p received message ~p~n", [Id, Msg]), %mensaje por pantalla
            NewTimes = Times - 1, % le quita 1 al contador
            if NewTimes > 0 -> % si quedan pasos se sigue mandado el mensaje
                {DestPid, NewDir} = case Dir of % segun direccion actual mira a que procesi mandar emnsaje y la nueva direccion
                    forward when NextPid =/= none -> {NextPid, forward}; % si direccion es ahcia delante y existe siguiente proceso sigue avanzando
                    forward when NextPid =:= none, PrevPid =/= none -> {PrevPid, backward}; % si va hacia delante pero no hay siguiente rebota, cambia direccion y manda mensaje a anterior
                    forward when NextPid =:= none, PrevPid =:= none -> {self(), backward}; % si no hay ni siguiente ni anterior se lo manda a si mismo
                    backward when PrevPid =/= none -> {PrevPid, backward}; % si va hacia atras y existe proceso anterior sigue hacia atras
                    backward when PrevPid =:= none, NextPid =/= none -> {NextPid, forward}; % si iba hacia atras pero llego al principio rebota y vuelve hacia delante
                    backward when PrevPid =:= none, NextPid =:= none -> {self(), forward} % si solo es un proceso
                end,
                DestPid ! {bounce, Msg, NewTimes, NewDir}; % envia mensaje al proceso que tocase
               true ->
                ok
            end,
            loop(Id, NextPid, PrevPid); % deja esperando mas mensajes
            
        stop -> % si proceso recibe stop se lo manda a sigueinte si tiene y termina
            if NextPid =/= none -> NextPid ! stop; % si tiene siguiente manda stop
               true -> ok
            end,
            ok
    end.

send(Pid, Msg) -> % pid del proceso y mensaje
    Pid ! {send, Msg},
    ok.

bounce(Pid, Msg, Times) ->
    if Times > 0 -> Pid ! {bounce, Msg, Times, forward};
       true -> ok
    end,
    ok.

stop(Pid) ->
    Pid ! stop,
    ok.
