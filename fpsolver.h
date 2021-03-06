#ifndef FPSOLVER_H
#define FPSOLVER_H

#include "Globals.h"
#include "body.h"
#include<string>
#include<fstream>

typedef std::map<int,Body> domain; //  all the balls in the current processor
typedef std::vector<vector3> vv3;

//ofstream myfile ("contacts.txt");


double compute_psi(Body &b1, Body &b2)
{
    double psi;
    vector3 diff = b1.get_position()-b2.get_position();
	double dist = diff.norml();
	psi = dist-(b1.get_rad() + b2.get_rad());
    return psi;
    //std::cout<<" \n psi is "<<psi<<std::endl;
}

void find_contact(domain &bodies,vector<pair<int,int> > &allcontacts)
{
	vector3 pos1, pos2, diff;
    int id1,id2;
    tr(bodies,it)
	{
        tr(bodies,it2)
		{
            id1 = it->first;
            id2 = it2->first;
            // make sure that the same sphere will not collide with itself
            if(id1<id2) 
            {
                pos1 = (it->second).get_position();
                pos2 = (it2->second).get_position();
                diff = pos1 - pos2;
                //if(diff.norml() - ( (it->second).get_rad() + (it2->second).get_rad() ) < 1E-2)
                if((compute_psi(it->second, it2->second)) < 1E-2)
                {
                    //	ii <==> pair<int, int>
                    allcontacts.push_back(ii((it->second).get_id(),(it2->second).get_id()));
                    ofstream myfile ("contacts1.txt");
                    myfile << allcontacts.size() << endl;
                }
            }
        }
	}
}

void construct_mass(Body &b1,Body &b2,vd &mass)
{
    double m1,i1,m2,i2;
    m1 = b1.get_mass();
    m2 = b2.get_mass();
    i1 = b1.get_inertia();
    i2 = b2.get_inertia();
    //b2.print();
    REP(i,0,3) { mass.push_back(m1);   }
    REP(i,0,3) { mass.push_back(i1);   }
    REP(i,0,3) { mass.push_back(m2);   }
    REP(i,0,3) { mass.push_back(i2);   }
    //print_vd(mass);
}

void construct_gn(Body &b1,Body &b2,vd &gn)
{
	vector3 gn1, gn2, gn1_norm, gn2_norm;
	gn1 = b1.get_position() - b2.get_position();
	gn1_norm = gn1.Normalize();
	gn2_norm = - gn1_norm;
	REP(i,0,3){gn.push_back(gn1_norm[i]);}
	REP(i,0,3){gn.push_back(0);          }
	REP(i,0,3){gn.push_back(gn2_norm[i]);}
	REP(i,0,3){gn.push_back(0);          }
    //print_vd(gn);
}

void construct_ext(Body &b1,Body &b2,vd &ext)
{
    // for the pair solver, the Pext is a vector of dimension 12
    // first 6 elements are the force and torque of the body 1 in contact
    // second 6 elements are the force and torque of the body 1 in contact
	vector3 ext1, ext2;
	ext1 = b1.get_ext();
	ext2 = b2.get_ext();
	REP(i,0,3) {ext.push_back(ext1[i]);    }
	REP(i,0,3) {ext.push_back(0);          }
	REP(i,0,3) {ext.push_back(ext2[i]);    }
	REP(i,0,3) {ext.push_back(0);          }
    //print_vd(ext);
}

void construct_vel(Body &b1,Body &b2,vd &vel)
{
	vector3 vel1, vel2;
	vel1 = b1.get_velocity(); 
	vel2 = b2.get_velocity();
	
	REP(i,0,3){vel.push_back(vel1[i]);    }
	REP(i,0,3){vel.push_back(0);          }
	REP(i,0,3){vel.push_back(vel2[i]);    }
	REP(i,0,3){vel.push_back(0);          }
    //print_vd(vel);
}


// this is to calculate the inner factor in the fix_point iteration method
double inner_factor(const vd &gn, const vd &ext, const vd &vel, double pPlusOne)
{
    double ret=0;
    REP(i,0,gn.size())
    {
        ret += gn[i] * ( vel[i] + pPlusOne* gn[i] + ext[i]);
    }
    return ret;
}


bool converged(const double &a, const double &b, double ep=1E-03)
{
    double diff = a-b;
    diff = diff>0 ? diff : -1*diff;
    return diff < ep;
}
double iterative_step(double psi, const vd &gn, const vd &ext, const vd &vel)
{
    // initialize pnPlusOne
    double prev=-1; // this is the value that will be returned when the iteration converges
    double temp=0;
    while(true)
    {
        temp = prev;
        temp += (-0.5) * ( psi + inner_factor(gn,ext,vel,prev));
        if(temp<0)
            temp=0;
        if(converged(temp,prev))
            return temp;  // by return, the while loop will end by itself
        prev = temp;
    }
}

vector3 pair_solver(Body &b1,Body &b2)
{
    /* Update the information for these bodies
     * */
    // Step 1 : Construct the three vectors corresponding to Mass, External Force and the velocity
    vd mass,ext,vel,gn,velPlusOne;
    double psi,pnPlusOne;
    construct_mass(b1,b2,mass);
    construct_ext(b1,b2,ext);
    construct_vel(b1,b2,vel);
    construct_gn(b1,b2,gn);
    psi = compute_psi(b1,b2);
    pnPlusOne = iterative_step(psi,gn,ext,vel);
    //cout<<"pnPlusOne is "<<pnPlusOne<<endl;
    return vector3(pnPlusOne*gn[0],pnPlusOne*gn[1],pnPlusOne*gn[2]);
}
void total_force(const map<int,vv3> &table,map<int,vector3> &force)
{
    vector3 f(0.0,0.0,0.0);
    tr(table,it)
    {
        tr(it->second,it2)
        {
            f+=(*it2);
        }
        force.insert( make_pair<int, vector3 >(it->first, f) );
    }
}

void initialize_table(domain &mybodies, map<int,vv3> &table)
{
    vector3 f(0.0,0.0,-0.00000098); // the initial gravity force and horizontal force
    tr(mybodies,it)
    {
        //table.insert(make_pair(it->first,f));
        table[it->first].push_back(f);
    }
}
void solve(domain &bodies,map<int,vector3> &force,mii &return_ids)
{
    /*
     * Solve the forces and the velocities for the local and foreign bodies
     */
    vector3 gnpn;
    map<int,vv3 > table;
    vector<pair<int,int > > contacts;
    initialize_table(bodies,table);
    find_contact(bodies,contacts);

    ofstream my_file ("contacts.txt");
    my_file << contacts.size() << endl;

    tr(contacts,it)
    {
       gnpn = pair_solver(bodies[it->first],bodies[it->second]);
       table[it->first].push_back(gnpn);  // here it->first is the first one of the two contact bodies
       table[it->second].push_back(gnpn*-1); // here it->second is the second one of the two contact bodies, so they have the same force with different directions
    }
    // 
    total_force(table,force);
   // int dest;
    tr(table,it)
    {
        //cout<<bodies[it->first].get_y()<<" , " << floor(bodies[it->first].get_y())<<endl;
        // pair<bodyID, the rank the body is in>
        return_ids.insert(make_pair(it->first,floor(bodies[it->first].get_y())));
    }
}

#endif
