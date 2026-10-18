-module(ringc). % nombre del modulo

-export([start/1, send/3, stop/1]). % funciones disponibles fuera del modulo y el numero de argumentos

start(N) when N > 0 -> % crea el anillo de 1 o mas procesos
    Pid0 = spawn(fun() -> wait_setup(0) end), % crea primer proceso, wait setup hace que no entre en el bucle directamente si no que espera saber cual es su siguiente proceso
    setup_ring(1, N - 1, Pid0, Pid0), % crea resto del anillo desde proceso 1 hasta N-1, pone cual es el primero del anillo y el anterior al nuevo proceso
    Pid0. % devuelve pid del primer proceso

setup_ring(Id, MaxId, FirstPid, PrevPid) when Id > MaxId -> % cuando ya no hay mas procesos que crear 
    PrevPid ! {set_next, FirstPid}, % se le marca al ultimo proceso que el siguiente es el primero para cerrar el anillo
    ok;
setup_ring(Id, MaxId, FirstPid, PrevPid) -> % crea proceso nuevo y lo enlaza con el anterior
    Pid = spawn(fun() -> wait_setup(Id) end), % crea proceso con numero Id y lo deja esperando hasta que sepa cual es su siguiente
    PrevPid ! {set_next, Pid}, % al anterior se le manda mensaje indicando que su siguiente es este nuevo proceso
    setup_ring(Id + 1, MaxId, FirstPid, Pid). % llamada recursiva para seguir creando anillo

wait_setup(Id) -> % espera un mensaje con su siguiente proceso y lo mete en el bucle normal
    receive
        {set_next, NextPid} ->
            loop(Id, NextPid)
    end.

loop(Id, NextPid) -> 
    receive
        {send, NLeft, Msg} when NLeft > 0 -> % caso normal de enviar mensaje por el anillo. Tipo mensaje, pasos que quedan y contenido
            io:format("~p receiving message ~p with ~p left~n", [Id, Msg, NLeft - 1]), % imprime por pantalla quien recive el mensaje, con que contenido y con cuantos pasos restantes (imprime n-1 porque el rproceso actual ya consume un paso)
            if NLeft - 1 > 0 -> % si quedan pasos
                NextPid ! {send, NLeft - 1, Msg}; % se le manda el mensaje al siguiente proceso con un paso menos
               true -> ok
            end,
            loop(Id, NextPid); % proceso vuelve a esperar mas mensajes
            
        {send, NLeft, _Msg} when NLeft =< 0 -> % si recibe mensaje con pasos 0 o negativo _Msg para evitar warning por unused
            loop(Id, NextPid); % no hace nada y lo vuelve a dejar en espera
            
        stop ->
            NextPid ! {stop_forward, Id}, % le manda a siguiente proceso el stop forward y se guarda el id del que inicio la parada para saber cuandose da la vuelta
            ok;
            
        {stop_forward, OriginId} -> % si no es proceso origen lo pasa y termina
            if OriginId =/= Id -> % si no es el de origen
                NextPid ! {stop_forward, OriginId}; % lo manda a siguiente y termina
               true -> ok
            end,
            ok
    end.

send(Pid, N, Msg) ->
    Pid ! {send, N, Msg},
    ok.

stop(Pid) ->
    Pid ! stop,
    ok.

