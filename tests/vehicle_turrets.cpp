#include "catch/catch.hpp"

#include "game.h"
#include "itype.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "player.h"

static std::vector<const vpart_info *> turret_types() {
    std::vector<const vpart_info *> res;
    
    auto parts = vpart_info::get_all();
    std::copy_if( parts.begin(), parts.end(), std::back_inserter( res ), []( const vpart_info *e ) {
        return e->has_flag( "TURRET" );
    } );

    return res;
}

const vpart_info *biggest_tank( const ammotype ammo ) {
    std::vector<const vpart_info *> res;
    
    auto parts = vpart_info::get_all();
    std::copy_if( parts.begin(), parts.end(), std::back_inserter( res ), [&ammo]( const vpart_info *e ) {

        if( !item( e->item ).is_watertight_container() ) {
            return false;
        }

        const itype *fuel = item::find_type( e->fuel_type );
        return fuel->ammo && fuel->ammo->type == ammo;
    } );

    if( res.empty() ) { 
        return nullptr;
    }

    return * std::max_element( res.begin(), res.end(), []( const vpart_info *lhs, const vpart_info *rhs ) {
        return lhs->size < rhs->size;
    } );
}

TEST_CASE( "vehicle_turret", "[vehicle] [gun] [magazine]" ) {
    for( auto e : turret_types() ) {
        SECTION( e->name() ) {
            vehicle *veh = g->m.add_vehicle( vproto_id( "none" ), 65, 65, 270, 0, 0 );
            REQUIRE( veh );

            const int idx = veh->install_part( 0, 0, e->id, true );
            REQUIRE( idx >= 0 );

            REQUIRE( veh->install_part( 0,  0, vpart_str_id( "storage_battery" ), true ) >= 0 );
            veh->charge_battery( 10000 );

            auto ammo = veh->turret_query( veh->parts[idx] ).base()->ammo_type();

            if( veh->part_flag( idx, "USE_TANKS" ) ) {
                auto *tank = biggest_tank( ammo );
                REQUIRE( tank );
                INFO( tank->id.str() );      

                auto tank_idx = veh->install_part( 0, 0, tank->id, true );
                REQUIRE( tank_idx >= 0 );
                REQUIRE( veh->parts[ tank_idx ].ammo_set( default_ammo( ammo ) ) );

            } else if( ammo ) {
                veh->parts[ idx].ammo_set( default_ammo( ammo ) );
            }

            auto qry = veh->turret_query( veh->parts[ idx ] );
            REQUIRE( qry );

            REQUIRE( qry.query() == turret_data::status::ready );
            REQUIRE( qry.range() > 0 );

            g->u.setpos( veh->global_part_pos3( idx ) );
            REQUIRE( qry.fire( g->u, g->u.pos() + point( qry.range(), 0 ) ) > 0 );

            g->m.destroy_vehicle( veh );
        }
    }
}
