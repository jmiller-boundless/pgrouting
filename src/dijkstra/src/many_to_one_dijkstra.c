/*PGR-GNU*****************************************************************

File: many_to_one_dijkstra.c

Generated with Template by:
Copyright (c) 2015 pgRouting developers
Mail: project@pgrouting.org

Function's developer: 
Copyright (c) 2015 Celia Virginia Vergara Castillo
Mail: vicky_vergara@hotmail.com

------

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

********************************************************************PGR-GNU*/

#include "./../../common/src/postgres_connection.h"
#include "utils/array.h"

#include "./../../common/src/debug_macro.h"
#include "./../../common/src/e_report.h"
#include "./../../common/src/time_msg.h"
#include "./../../common/src/pgr_types.h"
#include "./../../common/src/edges_input.h"
#include "./../../common/src/arrays_input.h"
#include "./many_to_many_dijkstra_driver.h"

PGDLLEXPORT Datum many_to_one_dijkstra(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(many_to_one_dijkstra);

static
void
process(
        char* edges_sql,
        ArrayType *starts,
        int64_t end_vid,
        bool directed,
        bool only_cost,
        General_path_element_t **result_tuples,
        size_t *result_count) {
    pgr_SPI_connect();

    int64_t* start_vidsArr = NULL;
    size_t size_start_vidsArr = 0;
    start_vidsArr = (int64_t*)
        pgr_get_bigIntArray(&size_start_vidsArr, starts);

    pgr_edge_t *edges = NULL;
    size_t total_edges = 0;
    pgr_get_edges_reversed(edges_sql, &edges, &total_edges);

    if (total_edges == 0) {
        if (start_vidsArr) pfree(start_vidsArr);
        pgr_SPI_finish();
        return;
    }

    PGR_DBG("Starting timer");
    clock_t start_t = clock();
    char* log_msg = NULL;
    char* notice_msg = NULL;
    char* err_msg = NULL;
    do_pgr_many_to_many_dijkstra(
            edges, total_edges,
            &end_vid, 1,
            start_vidsArr, size_start_vidsArr,

            directed,
            only_cost,
            false,  // data is reversed
            result_tuples,
            result_count,

            &log_msg,
            &notice_msg,
            &err_msg);

    if (only_cost) {
        time_msg("processing pgr_dijkstraCost(many to one)", start_t, clock());
    } else {
        time_msg("processing pgr_dijkstra(many to one)", start_t, clock());
    }

    if (err_msg && (*result_tuples)) {
        pfree(*result_tuples);
        (*result_count) = 0;
        (*result_tuples) = NULL;
    }

    pgr_global_report(log_msg, notice_msg, err_msg);

    if (log_msg) pfree(log_msg);
    if (notice_msg) pfree(notice_msg);
    if (err_msg) pfree(err_msg);
    if (edges) pfree(edges);
    if (start_vidsArr) pfree(start_vidsArr);

    pgr_SPI_finish();
}

PGDLLEXPORT Datum
many_to_one_dijkstra(PG_FUNCTION_ARGS) {
    FuncCallContext     *funcctx;
    TupleDesc            tuple_desc;

    /**********************************************************************/
    General_path_element_t  *result_tuples = NULL;
    size_t result_count = 0;
    /**********************************************************************/

    if (SRF_IS_FIRSTCALL()) {
        MemoryContext   oldcontext;
        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);


        /**********************************************************************/
        // pgr_dijkstra(
        // sql TEXT,
        // start_vids ANYARRAY,
        // end_vid BIGINT,
        // directed BOOLEAN default true,
        // only_cost BOOLEAN default false

        process(
                text_to_cstring(PG_GETARG_TEXT_P(0)),
                PG_GETARG_ARRAYTYPE_P(1),
                PG_GETARG_INT64(2),
                PG_GETARG_BOOL(3),
                PG_GETARG_BOOL(4),
                &result_tuples,
                &result_count);


#if PGSQL_VERSION > 95
        funcctx->max_calls = result_count;
#else
        funcctx->max_calls = (uint32_t)result_count;
#endif
        funcctx->user_fctx = result_tuples;
        if (get_call_result_type(fcinfo, NULL, &tuple_desc)
                != TYPEFUNC_COMPOSITE) {
            ereport(ERROR,
                    (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                     errmsg("function returning record called in context "
                         "that cannot accept type record")));
        }

        funcctx->tuple_desc = tuple_desc;
        MemoryContextSwitchTo(oldcontext);
    }

    funcctx = SRF_PERCALL_SETUP();
    tuple_desc = funcctx->tuple_desc;
    result_tuples = (General_path_element_t*) funcctx->user_fctx;

    if (funcctx->call_cntr < funcctx->max_calls) {
        HeapTuple    tuple;
        Datum        result;
        Datum        *values;
        bool*        nulls;
        size_t       call_cntr = funcctx->call_cntr;

        /**********************************************************************/
        // OUT seq INTEGER,
        // OUT path_seq INTEGER,
        // OUT start_vid BIGINT,
        // OUT node BIGINT,
        // OUT edge BIGINT,
        // OUT cost FLOAT,
        // OUT agg_cost FLOAT)

        size_t numb = 7;
        values = palloc(numb * sizeof(Datum));
        nulls = palloc(numb * sizeof(bool));

        size_t i;
        for (i = 0; i < numb; ++i) {
            nulls[i] = false;
        }

        values[0] = Int32GetDatum(call_cntr + 1);
        values[1] = Int32GetDatum(result_tuples[call_cntr].seq);
        values[2] = Int64GetDatum(result_tuples[call_cntr].start_id);
        values[3] = Int64GetDatum(result_tuples[call_cntr].node);
        values[4] = Int64GetDatum(result_tuples[call_cntr].edge);
        values[5] = Float8GetDatum(result_tuples[call_cntr].cost);
        values[6] = Float8GetDatum(result_tuples[call_cntr].agg_cost);
        /**********************************************************************/

        tuple = heap_form_tuple(tuple_desc, values, nulls);
        result = HeapTupleGetDatum(tuple);
        SRF_RETURN_NEXT(funcctx, result);
    } else {
        SRF_RETURN_DONE(funcctx);
    }
}

